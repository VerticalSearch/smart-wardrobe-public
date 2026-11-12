from __future__ import annotations

import copy
import json
import os
import threading
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SLOT_AVAILABLE = "available"
SLOT_RESERVED_STORE = "reserved-store"
SLOT_OCCUPIED = "occupied"
SLOT_RESERVED_FETCH = "reserved-fetch"
SLOT_ERROR = "error"

ACTIVE_TRANSACTION_STATES = {"reserved", "running"}
TERMINAL_TASK_STATES = {"done", "error", "stopped"}
STORE_AWAITING_CLOUD_ITEM = "awaiting-cloud-item"
LEGACY_MISSING_ITEM_ERROR = "store completed without item id"
TRUNCATED_COMMAND_ID_MIN_PREFIX_LEN = 16


class InventoryError(ValueError):
    pass


class WardrobeInventory:
    """Persistent C-slot ownership and store/fetch transaction state."""

    def __init__(self, path: Path, slot_count: int = 12) -> None:
        if slot_count <= 0:
            raise ValueError("slot_count must be positive")
        self.path = Path(path)
        self.slot_count = int(slot_count)
        self._lock = threading.RLock()
        with self._lock:
            self._data = self._load_and_normalize()
            self._save_locked()

    @staticmethod
    def _now() -> str:
        return datetime.now(timezone.utc).isoformat()

    def slot_label(self, index: int) -> str:
        if index < 0 or index >= self.slot_count:
            raise InventoryError(f"C slot index out of range: {index}")
        return f"C{index + 1:02d}"

    def normalize_slot(self, value: Any) -> str:
        if isinstance(value, bool):
            raise InventoryError("target_c must be C01-C12")
        if isinstance(value, int):
            number = value
        else:
            text = str(value or "").strip().upper()
            if text.startswith("C"):
                text = text[1:]
            try:
                number = int(text)
            except (TypeError, ValueError) as exc:
                raise InventoryError("target_c must be C01-C12") from exc
        if number < 1 or number > self.slot_count:
            raise InventoryError(f"target_c must be C01-C{self.slot_count:02d}")
        return self.slot_label(number - 1)

    def _empty_slot(self, index: int) -> dict[str, Any]:
        return {
            "index": index,
            "label": self.slot_label(index),
            "state": SLOT_AVAILABLE,
            "item_id": "",
            "transaction_id": "",
            "updated_at": self._now(),
        }

    def _empty_data(self) -> dict[str, Any]:
        return {
            "version": 2,
            "slot_count": self.slot_count,
            "slots": {self.slot_label(index): self._empty_slot(index) for index in range(self.slot_count)},
            "items": {},
            "transactions": {},
            "active_transaction": "",
            "updated_at": self._now(),
        }

    def _read_file(self) -> dict[str, Any]:
        try:
            data = json.loads(self.path.read_text(encoding="utf-8"))
        except (OSError, ValueError, TypeError):
            return {}
        return data if isinstance(data, dict) else {}

    def _load_and_normalize(self) -> dict[str, Any]:
        raw = self._read_file()
        if int(raw.get("version") or 0) >= 2 and isinstance(raw.get("slots"), dict):
            return self._normalize_v2(raw)
        return self._migrate_legacy(raw)

    def _normalize_v2(self, raw: dict[str, Any]) -> dict[str, Any]:
        data = self._empty_data()
        source_slots = raw.get("slots", {})
        allowed_states = {
            SLOT_AVAILABLE,
            SLOT_RESERVED_STORE,
            SLOT_OCCUPIED,
            SLOT_RESERVED_FETCH,
            SLOT_ERROR,
        }
        for index in range(self.slot_count):
            label = self.slot_label(index)
            source = source_slots.get(label, {})
            if not isinstance(source, dict):
                continue
            slot = data["slots"][label]
            state = str(source.get("state") or SLOT_AVAILABLE)
            slot["state"] = state if state in allowed_states else SLOT_ERROR
            slot["item_id"] = str(source.get("item_id") or "")
            slot["transaction_id"] = str(source.get("transaction_id") or "")
            slot["updated_at"] = str(source.get("updated_at") or self._now())

        items = raw.get("items")
        if isinstance(items, dict):
            for item_id, source in items.items():
                if not item_id or not isinstance(source, dict):
                    continue
                entry = dict(source)
                label = self._legacy_entry_label(entry)
                if label:
                    entry["label"] = label
                    entry["index"] = int(label[1:]) - 1
                data["items"][str(item_id)] = entry

        transactions = raw.get("transactions")
        if isinstance(transactions, dict):
            for command_id, source in transactions.items():
                if command_id and isinstance(source, dict):
                    data["transactions"][str(command_id)] = dict(source)
        data["active_transaction"] = str(raw.get("active_transaction") or "")
        data["updated_at"] = str(raw.get("updated_at") or self._now())
        self._repair_active_transaction(data)
        return data

    def _legacy_entry_label(self, entry: dict[str, Any]) -> str:
        if "label" in entry:
            label_value = entry.get("label")
            if label_value in (None, ""):
                return ""
            try:
                return self.normalize_slot(label_value)
            except InventoryError:
                return ""
        try:
            index = int(entry.get("index"))
        except (TypeError, ValueError):
            return ""
        if 0 <= index < self.slot_count:
            return self.slot_label(index)
        return ""

    def _migrate_legacy(self, raw: dict[str, Any]) -> dict[str, Any]:
        data = self._empty_data()
        items = raw.get("items") if isinstance(raw.get("items"), dict) else {}
        for item_id, source in items.items():
            if not item_id or not isinstance(source, dict):
                continue
            entry = dict(source)
            label = self._legacy_entry_label(entry)
            if not label:
                continue
            entry["index"] = int(label[1:]) - 1
            entry["label"] = label
            entry["state"] = str(entry.get("state") or "in-wardrobe")
            data["items"][str(item_id)] = entry
            slot = data["slots"][label]
            if slot["state"] == SLOT_AVAILABLE:
                slot["state"] = SLOT_OCCUPIED
                slot["item_id"] = str(item_id)
            elif slot["item_id"] != str(item_id):
                slot["state"] = SLOT_ERROR
                slot["error"] = "legacy duplicate slot mapping"
        try:
            legacy_next = max(0, min(self.slot_count, int(raw.get("next_c_index") or 0)))
        except (TypeError, ValueError):
            legacy_next = 0
        for index in range(legacy_next):
            slot = data["slots"][self.slot_label(index)]
            if slot["state"] == SLOT_AVAILABLE:
                slot["state"] = SLOT_ERROR
                slot["error"] = "legacy progress has no item mapping"
        data["migration"] = {
            "from": "items/next_c_index",
            "legacy_next_c_index": legacy_next,
            "migrated_at": self._now(),
        }
        return data

    @staticmethod
    def _repair_active_transaction(data: dict[str, Any]) -> None:
        command_id = str(data.get("active_transaction") or "")
        transaction = data.get("transactions", {}).get(command_id)
        if not isinstance(transaction, dict) or transaction.get("state") not in ACTIVE_TRANSACTION_STATES:
            data["active_transaction"] = ""
            return

        now = datetime.now(timezone.utc).isoformat()
        label = str(transaction.get("slot") or "")
        slot = data.get("slots", {}).get(label)
        error = "gateway restarted before a terminal board status was received"
        if isinstance(slot, dict):
            slot.update(state=SLOT_ERROR, transaction_id=command_id, updated_at=now)
            slot["error"] = error
        transaction.update(state="error", error=error, updated_at=now)
        data["active_transaction"] = ""

    def _save_locked(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self._data["updated_at"] = self._now()
        temp_path = self.path.with_name(f"{self.path.name}.tmp")
        temp_path.write_text(json.dumps(self._data, ensure_ascii=False, indent=2), encoding="utf-8")
        os.replace(temp_path, self.path)

    def snapshot(self) -> dict[str, Any]:
        with self._lock:
            return copy.deepcopy(self._data)

    def _active_locked(self) -> dict[str, Any] | None:
        command_id = str(self._data.get("active_transaction") or "")
        transaction = self._data["transactions"].get(command_id)
        if isinstance(transaction, dict) and transaction.get("state") in ACTIVE_TRANSACTION_STATES:
            return transaction
        self._data["active_transaction"] = ""
        return None

    def _ensure_no_active_locked(self, command_id: str) -> None:
        active = self._active_locked()
        if active is not None and active.get("id") != command_id:
            raise InventoryError(
                f"inventory transaction {active.get('id')} is still {active.get('state')} at {active.get('slot')}"
            )

    def _existing_transaction_locked(self, command_id: str, kind: str) -> dict[str, Any] | None:
        transaction = self._data["transactions"].get(command_id)
        if not isinstance(transaction, dict):
            return None
        if transaction.get("kind") != kind:
            raise InventoryError(f"command id {command_id} already belongs to {transaction.get('kind')}")
        return transaction

    def _new_transaction_locked(
        self,
        command_id: str,
        kind: str,
        action_id: int,
        slot_label: str,
        item_id: str = "",
    ) -> dict[str, Any]:
        now = self._now()
        transaction = {
            "id": command_id,
            "kind": kind,
            "action_id": int(action_id),
            "slot": slot_label,
            "slot_index": int(slot_label[1:]) - 1,
            "item_id": item_id,
            "state": "reserved",
            "cloud_sync": {"state": "not-needed", "status": "", "error": ""},
            "created_at": now,
            "updated_at": now,
        }
        self._data["transactions"][command_id] = transaction
        self._data["active_transaction"] = command_id
        return transaction

    def reserve_store(self, command_id: str, action_id: int, target_c: Any = None) -> dict[str, Any]:
        command_id = str(command_id or "").strip()
        if not command_id:
            raise InventoryError("store command id is required")
        with self._lock:
            existing = self._existing_transaction_locked(command_id, "store")
            if existing is not None:
                if existing.get("state") in ACTIVE_TRANSACTION_STATES:
                    return copy.deepcopy(existing)
                raise InventoryError(f"store command {command_id} is already {existing.get('state')}")
            self._ensure_no_active_locked(command_id)

            if target_c not in (None, "", 0, "0"):
                label = self.normalize_slot(target_c)
                candidates = [label]
            else:
                candidates = [self.slot_label(index) for index in range(self.slot_count)]
            label = next(
                (candidate for candidate in candidates if self._data["slots"][candidate]["state"] == SLOT_AVAILABLE),
                "",
            )
            if not label:
                if len(candidates) == 1:
                    raise InventoryError(f"target slot {candidates[0]} is not available")
                raise InventoryError("no available C slot")

            transaction = self._new_transaction_locked(command_id, "store", action_id, label)
            slot = self._data["slots"][label]
            slot.update(
                state=SLOT_RESERVED_STORE,
                item_id="",
                transaction_id=command_id,
                updated_at=self._now(),
            )
            self._save_locked()
            return copy.deepcopy(transaction)

    def reserve_fetch(
        self,
        command_id: str,
        action_id: int,
        item_id: str = "",
        target_c: Any = None,
    ) -> dict[str, Any]:
        command_id = str(command_id or "").strip()
        item_id = str(item_id or "").strip()
        if not command_id:
            raise InventoryError("fetch command id is required")
        with self._lock:
            existing = self._existing_transaction_locked(command_id, "fetch")
            if existing is not None:
                if existing.get("state") in ACTIVE_TRANSACTION_STATES:
                    return copy.deepcopy(existing)
                raise InventoryError(f"fetch command {command_id} is already {existing.get('state')}")
            self._ensure_no_active_locked(command_id)

            item_entry = self._data["items"].get(item_id) if item_id else None
            item_label = self._legacy_entry_label(item_entry) if isinstance(item_entry, dict) else ""
            target_label = self.normalize_slot(target_c) if target_c not in (None, "", 0, "0") else ""
            if item_label and target_label and item_label != target_label:
                raise InventoryError(f"item {item_id} belongs to {item_label}, not {target_label}")
            label = target_label or item_label
            if not label:
                raise InventoryError("fetch requires an occupied item_id or target_c")

            slot = self._data["slots"][label]
            if slot["state"] != SLOT_OCCUPIED:
                raise InventoryError(f"target slot {label} is not occupied")
            slot_item_id = str(slot.get("item_id") or "")
            if item_id and slot_item_id and item_id != slot_item_id:
                raise InventoryError(f"target slot {label} contains {slot_item_id}, not {item_id}")
            item_id = item_id or slot_item_id
            if not item_id:
                raise InventoryError(f"target slot {label} has no item mapping")

            transaction = self._new_transaction_locked(command_id, "fetch", action_id, label, item_id)
            slot.update(
                state=SLOT_RESERVED_FETCH,
                transaction_id=command_id,
                updated_at=self._now(),
            )
            self._save_locked()
            return copy.deepcopy(transaction)

    def _commit_store_locked(
        self,
        transaction: dict[str, Any],
        item_id: str,
        *,
        late_bound: bool = False,
    ) -> None:
        label = str(transaction["slot"])
        now = self._now()
        slot = self._data["slots"][label]
        slot.update(state=SLOT_OCCUPIED, item_id=item_id, transaction_id="", updated_at=now)
        slot.pop("error", None)
        item = self._data["items"].setdefault(item_id, {})
        item.update(
            index=int(label[1:]) - 1,
            label=label,
            state="in-wardrobe",
            updated_at=now,
        )
        transaction.update(item_id=item_id, state="done", updated_at=now)
        if late_bound:
            transaction["late_bound_at"] = now
        transaction.pop("error", None)
        transaction["cloud_sync"] = {
            "state": "pending",
            "status": "in-wardrobe",
            "real_position": label,
            "real_position_index": int(label[1:]) - 1,
            "error": "",
            "updated_at": now,
        }
        if self._data.get("active_transaction") == transaction.get("id"):
            self._data["active_transaction"] = ""

    @staticmethod
    def _accepts_late_store_item(transaction: dict[str, Any]) -> bool:
        if transaction.get("state") == STORE_AWAITING_CLOUD_ITEM:
            return True
        return (
            transaction.get("state") == "error"
            and str(transaction.get("error") or "") == LEGACY_MISSING_ITEM_ERROR
        )

    def _bind_store_item_locked(
        self,
        transaction: dict[str, Any],
        item_id: str,
        metadata: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        item_id = str(item_id or "").strip()
        if not item_id:
            raise InventoryError("store item id is required")
        if transaction.get("kind") != "store":
            raise InventoryError(f"transaction {transaction.get('id')} is not a store transaction")
        transaction_state = str(transaction.get("state") or "")
        if transaction_state not in ACTIVE_TRANSACTION_STATES and not self._accepts_late_store_item(transaction):
            raise InventoryError(f"store transaction {transaction.get('id')} is already {transaction_state}")
        existing_item_id = str(transaction.get("item_id") or "")
        if existing_item_id and existing_item_id != item_id:
            raise InventoryError(f"store transaction already bound to item {existing_item_id}")

        label = str(transaction["slot"])
        entry = dict(self._data["items"].get(item_id) or {})
        existing_label = self._legacy_entry_label(entry)
        if existing_label and existing_label != label:
            raise InventoryError(f"item {item_id} already belongs to {existing_label}, not {label}")
        owned_labels = [
            slot_label
            for slot_label, slot in self._data["slots"].items()
            if slot_label != label
            and str(slot.get("item_id") or "") == item_id
            and slot.get("state") != SLOT_AVAILABLE
        ]
        if owned_labels:
            raise InventoryError(f"item {item_id} is already owned by {','.join(owned_labels)}")

        entry.update(dict(metadata or {}))
        entry.update(
            index=int(label[1:]) - 1,
            label=label,
            state="pending-store",
            updated_at=self._now(),
        )
        self._data["items"][item_id] = entry
        transaction["item_id"] = item_id
        transaction["updated_at"] = self._now()
        self._data["slots"][label]["item_id"] = item_id
        self._data["slots"][label]["updated_at"] = self._now()
        if self._accepts_late_store_item(transaction):
            self._commit_store_locked(transaction, item_id, late_bound=True)
            entry = dict(self._data["items"][item_id])
        self._save_locked()
        return copy.deepcopy(entry)

    def bind_active_store_item(self, item_id: str, metadata: dict[str, Any] | None = None) -> dict[str, Any]:
        with self._lock:
            transaction = self._active_locked()
            if transaction is None or transaction.get("kind") != "store":
                raise InventoryError("no active store transaction for uploaded item")
            return self._bind_store_item_locked(transaction, item_id, metadata)

    def bind_store_item(
        self,
        command_id: str,
        item_id: str,
        metadata: dict[str, Any] | None = None,
    ) -> dict[str, Any]:
        command_id = str(command_id or "").strip()
        if not command_id:
            raise InventoryError("store command id is required for item binding")
        with self._lock:
            transaction = self._data["transactions"].get(command_id)
            if not isinstance(transaction, dict):
                raise InventoryError(f"store transaction {command_id} was not found")
            return self._bind_store_item_locked(transaction, item_id, metadata)

    def active_store_context(self) -> dict[str, Any] | None:
        with self._lock:
            transaction = self._active_locked()
            if transaction is None or transaction.get("kind") != "store":
                return None
            label = str(transaction["slot"])
            return {
                "command_id": str(transaction.get("id") or ""),
                "item_id": str(transaction.get("item_id") or ""),
                "real_position": label,
                "real_position_index": int(label[1:]) - 1,
            }

    def update_item_metadata(self, item_id: str, **metadata: Any) -> dict[str, Any] | None:
        item_id = str(item_id or "").strip()
        if not item_id:
            return None
        with self._lock:
            entry = self._data["items"].get(item_id)
            if not isinstance(entry, dict):
                return None
            for key, value in metadata.items():
                if value not in (None, ""):
                    entry[key] = value
            entry["updated_at"] = self._now()
            self._save_locked()
            return copy.deepcopy(entry)

    def mark_dispatched(self, command_id: str) -> dict[str, Any] | None:
        with self._lock:
            transaction = self._data["transactions"].get(str(command_id or ""))
            if not isinstance(transaction, dict) or transaction.get("state") not in ACTIVE_TRANSACTION_STATES:
                return None
            transaction["state"] = "running"
            transaction["updated_at"] = self._now()
            self._save_locked()
            return copy.deepcopy(transaction)

    def cancel_dispatch(self, command_id: str, error: str) -> dict[str, Any] | None:
        with self._lock:
            transaction = self._data["transactions"].get(str(command_id or ""))
            if not isinstance(transaction, dict) or transaction.get("state") not in ACTIVE_TRANSACTION_STATES:
                return None
            slot = self._data["slots"][transaction["slot"]]
            if transaction["kind"] == "store":
                slot.update(state=SLOT_AVAILABLE, item_id="", transaction_id="")
                item_id = str(transaction.get("item_id") or "")
                if item_id:
                    item = self._data["items"].setdefault(item_id, {})
                    item.update(state="store-error", last_label=transaction["slot"], updated_at=self._now())
                    item.pop("label", None)
                    item.pop("index", None)
            else:
                slot.update(state=SLOT_OCCUPIED, transaction_id="")
            slot["updated_at"] = self._now()
            transaction.update(state="send-failed", error=str(error), updated_at=self._now())
            if self._data.get("active_transaction") == transaction.get("id"):
                self._data["active_transaction"] = ""
            self._save_locked()
            return copy.deepcopy(transaction)

    def apply_task_status(
        self,
        command_id: str,
        action_id: int | None,
        state: str,
        target_c: Any = None,
    ) -> dict[str, Any] | None:
        command_id = str(command_id or "").strip()
        state = str(state or "").strip().lower()
        with self._lock:
            transaction = self._data["transactions"].get(command_id)
            resolved_command_id = command_id
            if not isinstance(transaction, dict) and len(command_id) >= TRUNCATED_COMMAND_ID_MIN_PREFIX_LEN:
                active_command_id = str(self._data.get("active_transaction") or "")
                active_transaction = self._data["transactions"].get(active_command_id)
                if (
                    isinstance(active_transaction, dict)
                    and active_transaction.get("state") in ACTIVE_TRANSACTION_STATES
                    and active_command_id.startswith(command_id)
                ):
                    transaction = active_transaction
                    resolved_command_id = active_command_id
            if not isinstance(transaction, dict):
                return None
            if action_id is not None and int(transaction.get("action_id") or 0) != int(action_id):
                raise InventoryError(f"task action mismatch for {resolved_command_id}")
            if target_c not in (None, "", 0, "0") and self.normalize_slot(target_c) != transaction.get("slot"):
                raise InventoryError(f"task target mismatch for {resolved_command_id}")
            if transaction.get("state") in {"done", "error", "stopped", "send-failed", "recovered"}:
                return copy.deepcopy(transaction)

            if state in {"received", "forwarded", "running", "sent"}:
                transaction["state"] = "running"
                transaction["updated_at"] = self._now()
                self._save_locked()
                return copy.deepcopy(transaction)
            if state not in TERMINAL_TASK_STATES:
                return copy.deepcopy(transaction)

            label = str(transaction["slot"])
            slot = self._data["slots"][label]
            item_id = str(transaction.get("item_id") or "")
            if state == "done" and transaction.get("kind") == "store" and item_id:
                self._commit_store_locked(transaction, item_id)
            elif state == "done" and transaction.get("kind") == "store":
                slot.update(
                    state=SLOT_RESERVED_STORE,
                    item_id="",
                    transaction_id=resolved_command_id,
                    updated_at=self._now(),
                )
                slot.pop("error", None)
                transaction.update(
                    state=STORE_AWAITING_CLOUD_ITEM,
                    hardware_done_at=self._now(),
                    updated_at=self._now(),
                )
                transaction.pop("error", None)
            elif state == "done" and transaction.get("kind") == "fetch":
                slot.update(state=SLOT_AVAILABLE, item_id="", transaction_id="")
                item = self._data["items"].setdefault(item_id, {})
                item.update(state="in-use", last_label=label, updated_at=self._now())
                item.pop("label", None)
                item.pop("index", None)
                transaction["state"] = "done"
                transaction["cloud_sync"] = {
                    "state": "pending",
                    "status": "in-use",
                    "real_position": label,
                    "real_position_index": int(label[1:]) - 1,
                    "error": "",
                    "updated_at": self._now(),
                }
            else:
                slot.update(state=SLOT_ERROR, transaction_id=resolved_command_id)
                slot["error"] = state
                transaction["state"] = state
                transaction["error"] = str(slot["error"])
                if item_id and transaction.get("kind") == "store":
                    item = self._data["items"].setdefault(item_id, {})
                    item.update(state="store-error", last_label=transaction["slot"], updated_at=self._now())
                    item.pop("label", None)
                    item.pop("index", None)
                    transaction["cloud_sync"] = {
                        "state": "pending",
                        "status": "store-error",
                        "real_position": label,
                        "real_position_index": int(label[1:]) - 1,
                        "error": "",
                        "updated_at": self._now(),
                    }

            slot["updated_at"] = self._now()
            transaction["updated_at"] = self._now()
            if self._data.get("active_transaction") == resolved_command_id:
                self._data["active_transaction"] = ""
            self._save_locked()
            return copy.deepcopy(transaction)

    def annotate_items(self, items: list[dict[str, Any]]) -> list[dict[str, Any]]:
        with self._lock:
            mapping = copy.deepcopy(self._data["items"])
        for item in items:
            item_id = str(item.get("id") or "")
            entry = mapping.get(item_id)
            if not isinstance(entry, dict):
                continue
            label = str(entry.get("label") or "")
            if label:
                item["real_position"] = label
                item["real_position_index"] = entry.get("index")
            elif entry.get("last_label"):
                item["last_real_position"] = entry.get("last_label")
            if entry.get("style_id") and item.get("style_id") in (None, 0, "0"):
                item["style_id"] = int(entry["style_id"])
            if entry.get("style") and not item.get("style"):
                item["style"] = entry["style"]
            local_state = str(entry.get("state") or "")
            if local_state in {"in-wardrobe", "in-use", "pending-store", "store-error", "deleted"}:
                item["status"] = local_state
        return items

    @staticmethod
    def _normalize_item_ids(item_ids: list[Any]) -> list[str]:
        normalized: list[str] = []
        seen: set[str] = set()
        for value in item_ids:
            item_id = str(value or "").strip()
            if not item_id or item_id in seen:
                continue
            seen.add(item_id)
            normalized.append(item_id)
        if not normalized:
            raise InventoryError("at least one wardrobe item id is required")
        return normalized

    def validate_cloud_delete(self, item_ids: list[Any]) -> list[str]:
        """Reject cloud deletion while a target item has uncertain physical state."""
        normalized = self._normalize_item_ids(item_ids)
        target_ids = set(normalized)
        with self._lock:
            active = self._active_locked()
            if isinstance(active, dict) and str(active.get("item_id") or "") in target_ids:
                raise InventoryError(
                    f"item {active.get('item_id')} is in active {active.get('kind')} transaction {active.get('id')}"
                )

            for label, slot in self._data["slots"].items():
                item_id = str(slot.get("item_id") or "")
                if item_id not in target_ids:
                    continue
                state = str(slot.get("state") or "")
                if state in {SLOT_RESERVED_STORE, SLOT_RESERVED_FETCH, SLOT_ERROR}:
                    raise InventoryError(f"item {item_id} at {label} cannot be deleted while slot is {state}")

            for item_id in normalized:
                entry = self._data["items"].get(item_id)
                if not isinstance(entry, dict):
                    continue
                label = self._legacy_entry_label(entry)
                if not label:
                    continue
                slot = self._data["slots"][label]
                slot_item_id = str(slot.get("item_id") or "")
                if slot_item_id and slot_item_id != item_id:
                    raise InventoryError(
                        f"item {item_id} maps to {label}, but the slot contains {slot_item_id}"
                    )
        return normalized

    def mark_cloud_items_deleted(self, item_ids: list[Any], reason: str = "cloud-delete") -> dict[str, Any]:
        """Release ordinary occupied slots and keep tombstones against stale cloud reads."""
        normalized = self.validate_cloud_delete(item_ids)
        target_ids = set(normalized)
        released_slots: list[dict[str, str]] = []
        cancelled_updates: list[dict[str, str]] = []
        with self._lock:
            now = self._now()
            labels_by_item: dict[str, list[str]] = {item_id: [] for item_id in normalized}
            for label, slot in self._data["slots"].items():
                item_id = str(slot.get("item_id") or "")
                if item_id not in target_ids:
                    continue
                if slot.get("state") == SLOT_AVAILABLE:
                    slot.update(item_id="", transaction_id="", updated_at=now)
                    continue
                if slot.get("state") != SLOT_OCCUPIED:
                    raise InventoryError(f"item {item_id} at {label} changed state during cloud deletion")
                labels_by_item[item_id].append(label)
                slot.update(
                    state=SLOT_AVAILABLE,
                    item_id="",
                    transaction_id="",
                    updated_at=now,
                )
                slot.pop("error", None)
                released_slots.append({"item_id": item_id, "slot": label})

            for item_id in normalized:
                entry = self._data["items"].setdefault(item_id, {})
                prior_label = self._legacy_entry_label(entry)
                labels = labels_by_item[item_id]
                last_label = prior_label or (labels[0] if labels else str(entry.get("last_label") or ""))
                entry.update(
                    state="deleted",
                    deleted_at=now,
                    delete_reason=str(reason or "cloud-delete"),
                    updated_at=now,
                )
                if last_label:
                    entry["last_label"] = last_label
                entry.pop("label", None)
                entry.pop("index", None)

            for command_id, transaction in self._data["transactions"].items():
                item_id = str(transaction.get("item_id") or "")
                if item_id not in target_ids:
                    continue
                cloud_sync = transaction.get("cloud_sync")
                if not isinstance(cloud_sync, dict) or cloud_sync.get("state") not in {"pending", "retry"}:
                    continue
                cloud_sync.update(
                    state="cancelled",
                    error="superseded by cloud deletion",
                    updated_at=now,
                )
                cancelled_updates.append({"command_id": str(command_id), "item_id": item_id})

            self._save_locked()
        return {
            "item_ids": normalized,
            "released_slots": released_slots,
            "cancelled_cloud_updates": cancelled_updates,
        }

    def reconcile_cloud_items(self, items: list[dict[str, Any]]) -> dict[str, Any]:
        imported: list[dict[str, str]] = []
        recovered: list[dict[str, str]] = []
        conflicts: list[dict[str, str]] = []
        changed = False
        with self._lock:
            late_candidates: dict[str, list[tuple[dict[str, Any], str]]] = {}
            for cloud_item in items:
                if not isinstance(cloud_item, dict):
                    continue
                if str(cloud_item.get("status") or "").strip().lower() != "in-wardrobe":
                    continue
                command_id = str(cloud_item.get("capture_session_id") or "").strip()
                item_id = str(cloud_item.get("id") or "").strip()
                target = cloud_item.get("real_position", cloud_item.get("slot_id"))
                if not command_id or not item_id or target in (None, ""):
                    continue
                try:
                    label = self.normalize_slot(target)
                except InventoryError:
                    continue
                late_candidates.setdefault(command_id, []).append((cloud_item, label))

            for command_id, candidates in late_candidates.items():
                transaction = self._data["transactions"].get(command_id)
                if not isinstance(transaction, dict) or not self._accepts_late_store_item(transaction):
                    continue
                expected_label = str(transaction.get("slot") or "")
                exact_candidates = [candidate for candidate in candidates if candidate[1] == expected_label]
                if len(candidates) != 1 or len(exact_candidates) != 1:
                    conflicts.append(
                        {
                            "item_id": ",".join(str(candidate[0].get("id") or "") for candidate in candidates),
                            "slot": expected_label,
                            "local_item_id": str(transaction.get("item_id") or ""),
                            "local_state": "ambiguous-late-cloud-item",
                            "transaction_id": command_id,
                        }
                    )
                    continue
                cloud_item, label = exact_candidates[0]
                item_id = str(cloud_item.get("id") or "").strip()
                style = str(cloud_item.get("style") or cloud_item.get("category") or cloud_item.get("type") or "")
                metadata: dict[str, Any] = {
                    "capture_session_id": command_id,
                    "late_cloud_bind": True,
                }
                if style:
                    metadata["style"] = style
                try:
                    self._bind_store_item_locked(transaction, item_id, metadata)
                except InventoryError as exc:
                    conflicts.append(
                        {
                            "item_id": item_id,
                            "slot": label,
                            "local_item_id": str(transaction.get("item_id") or ""),
                            "local_state": "late-cloud-bind-rejected",
                            "transaction_id": command_id,
                            "error": str(exc),
                        }
                    )
                    continue
                recovered.append({"item_id": item_id, "slot": label, "transaction_id": command_id})
                changed = True

            for cloud_item in items:
                if not isinstance(cloud_item, dict):
                    continue
                if str(cloud_item.get("status") or "").strip().lower() != "in-wardrobe":
                    continue
                item_id = str(cloud_item.get("id") or "").strip()
                target = cloud_item.get("real_position", cloud_item.get("slot_id"))
                if not item_id or target in (None, ""):
                    continue
                try:
                    label = self.normalize_slot(target)
                except InventoryError:
                    continue

                local_entry = self._data["items"].get(item_id)
                if isinstance(local_entry, dict) and str(local_entry.get("state") or "") not in {"", "in-wardrobe"}:
                    continue

                local_label = self._legacy_entry_label(local_entry) if isinstance(local_entry, dict) else ""
                owned_labels = [
                    local_slot_label
                    for local_slot_label, local_slot in self._data["slots"].items()
                    if str(local_slot.get("item_id") or "") == item_id
                    and local_slot.get("state") != SLOT_AVAILABLE
                ]
                if len(owned_labels) > 1:
                    conflicts.append(
                        {
                            "item_id": item_id,
                            "slot": label,
                            "local_item_id": item_id,
                            "local_state": "duplicate-local-item",
                            "local_slot": ",".join(owned_labels),
                        }
                    )
                    continue
                if local_label and local_label != label:
                    conflicts.append(
                        {
                            "item_id": item_id,
                            "slot": label,
                            "local_item_id": item_id,
                            "local_state": str(local_entry.get("state") or "in-wardrobe"),
                            "local_slot": local_label,
                        }
                    )
                    continue
                if owned_labels and owned_labels[0] != label:
                    conflicts.append(
                        {
                            "item_id": item_id,
                            "slot": label,
                            "local_item_id": item_id,
                            "local_state": "owned-by-other-local-slot",
                            "local_slot": owned_labels[0],
                        }
                    )
                    continue

                slot = self._data["slots"][label]
                slot_item_id = str(slot.get("item_id") or "")
                if slot.get("state") == SLOT_AVAILABLE:
                    slot.update(
                        state=SLOT_OCCUPIED,
                        item_id=item_id,
                        transaction_id="",
                        updated_at=self._now(),
                    )
                    imported.append({"item_id": item_id, "slot": label})
                    changed = True
                elif slot.get("state") == SLOT_OCCUPIED and slot_item_id == item_id:
                    pass
                elif slot.get("state") in {SLOT_RESERVED_STORE, SLOT_RESERVED_FETCH} and slot_item_id == item_id:
                    continue
                else:
                    conflicts.append(
                        {
                            "item_id": item_id,
                            "slot": label,
                            "local_item_id": slot_item_id,
                            "local_state": str(slot.get("state") or ""),
                        }
                    )
                    continue

                entry = self._data["items"].setdefault(item_id, {})
                entry.update(
                    index=int(label[1:]) - 1,
                    label=label,
                    state="in-wardrobe",
                    updated_at=self._now(),
                )
                changed = True

            if changed:
                self._save_locked()
        return {"imported": imported, "recovered": recovered, "conflicts": conflicts}

    def pending_cloud_updates(self) -> list[dict[str, Any]]:
        with self._lock:
            pending = []
            for transaction in self._data["transactions"].values():
                cloud_sync = transaction.get("cloud_sync")
                if isinstance(cloud_sync, dict) and cloud_sync.get("state") in {"pending", "retry"}:
                    pending.append(
                        {
                            "command_id": transaction.get("id", ""),
                            "item_id": transaction.get("item_id", ""),
                            "status": cloud_sync.get("status", ""),
                            "real_position": cloud_sync.get("real_position", ""),
                            "real_position_index": cloud_sync.get("real_position_index"),
                        }
                    )
            return pending

    def mark_cloud_sync(self, command_id: str, ok: bool, error: str = "") -> None:
        with self._lock:
            transaction = self._data["transactions"].get(str(command_id or ""))
            if not isinstance(transaction, dict):
                return
            cloud_sync = transaction.get("cloud_sync")
            if not isinstance(cloud_sync, dict):
                return
            cloud_sync["state"] = "done" if ok else "retry"
            cloud_sync["error"] = "" if ok else str(error)
            cloud_sync["updated_at"] = self._now()
            self._save_locked()

    def resolve_error(self, command_id: str, outcome: str) -> dict[str, Any]:
        outcome = str(outcome or "").strip().lower()
        with self._lock:
            transaction = self._data["transactions"].get(str(command_id or ""))
            if not isinstance(transaction, dict) or transaction.get("state") not in {"error", "stopped"}:
                raise InventoryError("transaction is not waiting for error recovery")
            slot = self._data["slots"][transaction["slot"]]
            item_id = str(transaction.get("item_id") or "")
            if outcome == "available":
                slot.update(state=SLOT_AVAILABLE, item_id="", transaction_id="")
                if item_id and transaction.get("kind") == "store":
                    item = self._data["items"].setdefault(item_id, {})
                    item.update(state="store-error", last_label=transaction["slot"], updated_at=self._now())
                    item.pop("label", None)
                    item.pop("index", None)
                    transaction["cloud_sync"] = {
                        "state": "pending",
                        "status": "store-error",
                        "real_position": transaction["slot"],
                        "real_position_index": int(transaction["slot"][1:]) - 1,
                        "error": "",
                        "updated_at": self._now(),
                    }
                elif item_id:
                    item = self._data["items"].setdefault(item_id, {})
                    item.update(state="in-use", last_label=transaction["slot"], updated_at=self._now())
                    item.pop("label", None)
                    item.pop("index", None)
                    transaction["cloud_sync"] = {
                        "state": "pending",
                        "status": "in-use",
                        "real_position": transaction["slot"],
                        "real_position_index": int(transaction["slot"][1:]) - 1,
                        "error": "",
                        "updated_at": self._now(),
                    }
            elif outcome == "occupied":
                if not item_id:
                    raise InventoryError("occupied recovery requires a bound item id")
                slot.update(state=SLOT_OCCUPIED, item_id=item_id, transaction_id="")
                item = self._data["items"].setdefault(item_id, {})
                item.update(
                    index=int(transaction["slot"][1:]) - 1,
                    label=transaction["slot"],
                    state="in-wardrobe",
                    updated_at=self._now(),
                )
                transaction["cloud_sync"] = {
                    "state": "pending",
                    "status": "in-wardrobe",
                    "real_position": transaction["slot"],
                    "real_position_index": int(transaction["slot"][1:]) - 1,
                    "error": "",
                    "updated_at": self._now(),
                }
            else:
                raise InventoryError("outcome must be available or occupied")
            slot.pop("error", None)
            slot["updated_at"] = self._now()
            transaction["recovered_as"] = outcome
            transaction["state"] = "recovered"
            transaction["updated_at"] = self._now()
            self._save_locked()
            return copy.deepcopy(transaction)

    def resolve_slot_error(self, target_c: Any, outcome: str, item_id: str = "") -> dict[str, Any]:
        label = self.normalize_slot(target_c)
        outcome = str(outcome or "").strip().lower()
        item_id = str(item_id or "").strip()
        with self._lock:
            slot = self._data["slots"][label]
            if slot.get("state") != SLOT_ERROR:
                raise InventoryError(f"slot {label} is not waiting for error recovery")
            if slot.get("transaction_id"):
                raise InventoryError("recover this slot by command_id")
            if outcome == "available":
                slot.update(state=SLOT_AVAILABLE, item_id="", transaction_id="")
            elif outcome == "occupied":
                if not item_id:
                    raise InventoryError("occupied recovery requires item_id")
                slot.update(state=SLOT_OCCUPIED, item_id=item_id, transaction_id="")
                self._data["items"].setdefault(item_id, {}).update(
                    index=int(label[1:]) - 1,
                    label=label,
                    state="in-wardrobe",
                    updated_at=self._now(),
                )
            else:
                raise InventoryError("outcome must be available or occupied")
            slot.pop("error", None)
            slot["updated_at"] = self._now()
            self._save_locked()
            return copy.deepcopy(slot)

    def reset_all_available(self, reason: str = "operator inventory reset") -> dict[str, Any]:
        """Explicitly abandon all local C-slot ownership and restart at C01."""
        reason = str(reason or "operator inventory reset").strip()
        with self._lock:
            active_command_id = str(self._data.get("active_transaction") or "")
            if active_command_id:
                raise InventoryError(f"cannot reset inventory while transaction {active_command_id} is active")

            reset_slots: list[str] = []
            reset_items: list[str] = []
            recovered_transactions: list[str] = []
            now = self._now()
            for label, slot in self._data["slots"].items():
                item_id = str(slot.get("item_id") or "")
                transaction_id = str(slot.get("transaction_id") or "")
                if item_id:
                    item = self._data["items"].setdefault(item_id, {})
                    item.update(
                        state="deleted",
                        last_label=label,
                        deleted_at=now,
                        delete_reason="inventory-reset",
                        updated_at=now,
                    )
                    item.pop("label", None)
                    item.pop("index", None)
                    reset_items.append(item_id)
                if transaction_id:
                    transaction = self._data["transactions"].get(transaction_id)
                    if isinstance(transaction, dict) and transaction.get("state") not in {"done", "recovered"}:
                        transaction.update(
                            state="recovered",
                            recovered_as="available",
                            recovery_reason=reason,
                            updated_at=now,
                        )
                        recovered_transactions.append(transaction_id)
                slot.update(state=SLOT_AVAILABLE, item_id="", transaction_id="", updated_at=now)
                slot.pop("error", None)
                reset_slots.append(label)

            self._data["active_transaction"] = ""
            self._save_locked()
            return {
                "ok": True,
                "reason": reason,
                "reset_slots": reset_slots,
                "reset_items": sorted(set(reset_items)),
                "recovered_transactions": sorted(set(recovered_transactions)),
                "next_store_slot": self.slot_label(0),
            }
