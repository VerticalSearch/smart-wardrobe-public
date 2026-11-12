import json
from pathlib import Path

from wardrobe_gateway.inventory import (
    SLOT_AVAILABLE,
    SLOT_ERROR,
    SLOT_OCCUPIED,
    SLOT_RESERVED_FETCH,
    SLOT_RESERVED_STORE,
    STORE_AWAITING_CLOUD_ITEM,
    InventoryError,
    WardrobeInventory,
)


def inventory(tmp_path: Path) -> WardrobeInventory:
    return WardrobeInventory(tmp_path / "store_positions.json", slot_count=12)


def test_legacy_mapping_migrates_to_zero_padded_occupied_slot(tmp_path):
    path = tmp_path / "store_positions.json"
    path.write_text('{"items":{"shirt":{"index":0,"label":"C1"}},"next_c_index":1}', encoding="utf-8")

    store = WardrobeInventory(path)
    snapshot = store.snapshot()

    assert snapshot["version"] == 2
    assert snapshot["slots"]["C01"]["state"] == SLOT_OCCUPIED
    assert snapshot["slots"]["C01"]["item_id"] == "shirt"
    assert snapshot["items"]["shirt"]["label"] == "C01"


def test_legacy_progress_without_item_mapping_is_locked_for_review(tmp_path):
    path = tmp_path / "store_positions.json"
    path.write_text('{"items":{},"next_c_index":2}', encoding="utf-8")

    store = WardrobeInventory(path)
    assert store.snapshot()["slots"]["C01"]["state"] == SLOT_ERROR
    assert store.snapshot()["slots"]["C02"]["state"] == SLOT_ERROR
    assert store.reserve_store("store-3", 11)["slot"] == "C03"
    store.cancel_dispatch("store-3", "test")
    store.resolve_slot_error("C01", "available")
    assert store.snapshot()["slots"]["C01"]["state"] == SLOT_AVAILABLE


def test_legacy_explicit_empty_label_does_not_guess_position_from_default_index(tmp_path):
    path = tmp_path / "store_positions.json"
    path.write_text(
        json.dumps(
            {
                "items": {
                    "confirmed": {"index": 0, "label": "C1"},
                    "metadata-only": {"index": 0, "label": ""},
                },
                "next_c_index": 1,
            }
        ),
        encoding="utf-8",
    )

    store = WardrobeInventory(path)
    snapshot = store.snapshot()

    assert snapshot["slots"]["C01"]["state"] == SLOT_OCCUPIED
    assert snapshot["slots"]["C01"]["item_id"] == "confirmed"
    assert "metadata-only" not in snapshot["items"]


def test_store_transaction_persists_and_commits_only_after_done(tmp_path):
    path = tmp_path / "store_positions.json"
    store = WardrobeInventory(path)

    tx = store.reserve_store("store-1", 11)
    assert tx["slot"] == "C01"
    assert store.snapshot()["slots"]["C01"]["state"] == SLOT_RESERVED_STORE

    store.bind_active_store_item("item-1", {"style_id": 2, "style": "shirt"})
    store.apply_task_status("store-1", 11, "running", "C01")
    assert store.snapshot()["slots"]["C01"]["state"] == SLOT_RESERVED_STORE

    store.apply_task_status("store-1", 11, "done", "C01")
    restarted = WardrobeInventory(path)
    snapshot = restarted.snapshot()
    assert snapshot["slots"]["C01"]["state"] == SLOT_OCCUPIED
    assert snapshot["slots"]["C01"]["item_id"] == "item-1"
    assert snapshot["items"]["item-1"]["state"] == "in-wardrobe"
    assert restarted.pending_cloud_updates() == [
        {
            "command_id": "store-1",
            "item_id": "item-1",
            "status": "in-wardrobe",
            "real_position": "C01",
            "real_position_index": 0,
        }
    ]


def test_truncated_board_command_id_commits_active_store_transaction(tmp_path):
    store = inventory(tmp_path)
    full_command_id = "store-after-photo-task-msu96o4n"
    truncated_command_id = "store-after-photo-task-m"

    store.reserve_store(full_command_id, 11, "C01")
    store.bind_active_store_item("item-9070", {"style_id": 10, "style": "jacket"})
    store.mark_dispatched(full_command_id)

    transaction = store.apply_task_status(truncated_command_id, 11, "done", "C01")
    snapshot = store.snapshot()

    assert transaction is not None
    assert transaction["id"] == full_command_id
    assert transaction["state"] == "done"
    assert snapshot["active_transaction"] == ""
    assert snapshot["slots"]["C01"]["state"] == SLOT_OCCUPIED
    assert snapshot["slots"]["C01"]["item_id"] == "item-9070"
    assert snapshot["items"]["item-9070"]["state"] == "in-wardrobe"
    assert store.pending_cloud_updates() == [
        {
            "command_id": full_command_id,
            "item_id": "item-9070",
            "status": "in-wardrobe",
            "real_position": "C01",
            "real_position_index": 0,
        }
    ]


def test_truncated_board_command_id_still_checks_action_and_slot(tmp_path):
    store = inventory(tmp_path)
    full_command_id = "store-after-photo-task-msu96o4n"
    truncated_command_id = "store-after-photo-task-m"
    store.reserve_store(full_command_id, 11, "C01")
    store.mark_dispatched(full_command_id)

    try:
        store.apply_task_status(truncated_command_id, 7, "done", "C01")
    except InventoryError as exc:
        assert "task action mismatch" in str(exc)
    else:
        raise AssertionError("truncated status must not bypass action validation")

    try:
        store.apply_task_status(truncated_command_id, 11, "done", "C02")
    except InventoryError as exc:
        assert "task target mismatch" in str(exc)
    else:
        raise AssertionError("truncated status must not bypass target validation")


def test_store_done_without_item_waits_for_exact_late_cloud_binding(tmp_path):
    store = inventory(tmp_path)
    store.reserve_store("store-late-1", 11, "C04")
    store.mark_dispatched("store-late-1")

    transaction = store.apply_task_status("store-late-1", 11, "done", "C04")
    waiting = store.snapshot()

    assert transaction is not None
    assert transaction["state"] == STORE_AWAITING_CLOUD_ITEM
    assert waiting["active_transaction"] == ""
    assert waiting["slots"]["C04"]["state"] == SLOT_RESERVED_STORE
    assert waiting["slots"]["C04"]["transaction_id"] == "store-late-1"
    assert store.reserve_store("store-next", 11)["slot"] == "C01"
    store.cancel_dispatch("store-next", "test cleanup")

    result = store.reconcile_cloud_items(
        [
            {
                "id": "item-late-4",
                "status": "in-wardrobe",
                "real_position": "C04",
                "capture_session_id": "store-late-1",
                "category": "jacket",
            }
        ]
    )

    assert result["recovered"] == [
        {"item_id": "item-late-4", "slot": "C04", "transaction_id": "store-late-1"}
    ]
    recovered = store.snapshot()
    assert recovered["transactions"]["store-late-1"]["state"] == "done"
    assert recovered["slots"]["C04"]["state"] == SLOT_OCCUPIED
    assert recovered["slots"]["C04"]["item_id"] == "item-late-4"
    assert recovered["items"]["item-late-4"]["state"] == "in-wardrobe"


def test_late_cloud_binding_rejects_wrong_slot_and_duplicate_result(tmp_path):
    store = inventory(tmp_path)
    store.reserve_store("store-late-2", 11, "C05")
    store.mark_dispatched("store-late-2")
    store.apply_task_status("store-late-2", 11, "done", "C05")

    result = store.reconcile_cloud_items(
        [
            {
                "id": "item-a",
                "status": "in-wardrobe",
                "real_position": "C05",
                "capture_session_id": "store-late-2",
            },
            {
                "id": "item-b",
                "status": "in-wardrobe",
                "real_position": "C06",
                "capture_session_id": "store-late-2",
            },
        ]
    )

    assert result["recovered"] == []
    assert any(conflict["local_state"] == "ambiguous-late-cloud-item" for conflict in result["conflicts"])
    snapshot = store.snapshot()
    assert snapshot["transactions"]["store-late-2"]["state"] == STORE_AWAITING_CLOUD_ITEM
    assert snapshot["slots"]["C05"]["state"] == SLOT_RESERVED_STORE


def test_legacy_missing_item_error_is_recovered_only_by_exact_capture_session(tmp_path):
    path = tmp_path / "store_positions.json"
    store = WardrobeInventory(path)
    store.reserve_store("store-legacy-late", 11, "C08")
    store.mark_dispatched("store-legacy-late")
    damaged = store.snapshot()
    damaged["active_transaction"] = ""
    damaged["transactions"]["store-legacy-late"].update(
        state="error",
        error="store completed without item id",
    )
    damaged["slots"]["C08"].update(
        state=SLOT_ERROR,
        error="store completed without item id",
    )
    path.write_text(json.dumps(damaged), encoding="utf-8")
    restarted = WardrobeInventory(path)

    result = restarted.reconcile_cloud_items(
        [
            {
                "id": "item-legacy-8",
                "status": "in-wardrobe",
                "real_position": "C08",
                "capture_session_id": "store-legacy-late",
            }
        ]
    )

    assert result["recovered"] == [
        {"item_id": "item-legacy-8", "slot": "C08", "transaction_id": "store-legacy-late"}
    ]
    snapshot = restarted.snapshot()
    assert snapshot["slots"]["C08"]["state"] == SLOT_OCCUPIED
    assert snapshot["slots"]["C08"]["item_id"] == "item-legacy-8"


def test_store_automatically_allocates_next_available_slot(tmp_path):
    store = inventory(tmp_path)
    assert store.reserve_store("store-1", 11)["slot"] == "C01"
    store.bind_active_store_item("item-1")
    store.apply_task_status("store-1", 11, "done", "C01")
    store.mark_cloud_sync("store-1", True)

    assert store.reserve_store("store-2", 11)["slot"] == "C02"


def test_restart_moves_unfinished_transaction_to_recoverable_error(tmp_path):
    path = tmp_path / "store_positions.json"
    store = WardrobeInventory(path)
    store.reserve_store("store-1", 11, "C01")
    store.mark_dispatched("store-1")

    restarted = WardrobeInventory(path)
    snapshot = restarted.snapshot()
    assert snapshot["active_transaction"] == ""
    assert snapshot["transactions"]["store-1"]["state"] == "error"
    assert snapshot["slots"]["C01"]["state"] == SLOT_ERROR
    assert restarted.reserve_store("store-2", 11)["slot"] == "C02"


def test_late_done_does_not_override_manual_recovery(tmp_path):
    store = inventory(tmp_path)
    store.reserve_store("store-1", 11, "C01")
    store.bind_active_store_item("item-1")
    store.apply_task_status("store-1", 11, "error", "C01")
    store.resolve_error("store-1", "available")

    store.apply_task_status("store-1", 11, "done", "C01")
    snapshot = store.snapshot()
    assert snapshot["transactions"]["store-1"]["state"] == "recovered"
    assert snapshot["slots"]["C01"]["state"] == SLOT_AVAILABLE


def test_cloud_position_rebuilds_empty_local_inventory(tmp_path):
    store = inventory(tmp_path)
    result = store.reconcile_cloud_items(
        [
            {"id": "item-6", "status": "in-wardrobe", "real_position": "C6"},
            {"id": "item-out", "status": "in-use", "real_position": "C07"},
        ]
    )

    assert result["imported"] == [{"item_id": "item-6", "slot": "C06"}]
    assert result["conflicts"] == []
    snapshot = store.snapshot()
    assert snapshot["slots"]["C06"]["state"] == SLOT_OCCUPIED
    assert snapshot["slots"]["C06"]["item_id"] == "item-6"
    assert store.reserve_fetch("fetch-6", 7, item_id="item-6")["slot"] == "C06"


def test_stale_cloud_status_does_not_reoccupy_fetched_slot(tmp_path):
    store = inventory(tmp_path)
    store.reconcile_cloud_items([{"id": "item-5", "status": "in-wardrobe", "real_position": "C05"}])
    store.reserve_fetch("fetch-5", 7, item_id="item-5")
    store.apply_task_status("fetch-5", 7, "done", "C05")

    store.reconcile_cloud_items([{"id": "item-5", "status": "in-wardrobe", "real_position": "C05"}])
    snapshot = store.snapshot()
    assert snapshot["slots"]["C05"]["state"] == SLOT_AVAILABLE
    cloud_item = {"id": "item-5", "status": "in-wardrobe", "real_position": "C05"}
    store.annotate_items([cloud_item])
    assert cloud_item["status"] == "in-use"


def test_cloud_position_mismatch_does_not_duplicate_local_item(tmp_path):
    store = inventory(tmp_path)
    store.reconcile_cloud_items([{"id": "item-5", "status": "in-wardrobe", "real_position": "C05"}])

    result = store.reconcile_cloud_items(
        [{"id": "item-5", "status": "in-wardrobe", "real_position": "C06"}]
    )

    assert result["imported"] == []
    assert result["conflicts"] == [
        {
            "item_id": "item-5",
            "slot": "C06",
            "local_item_id": "item-5",
            "local_state": "in-wardrobe",
            "local_slot": "C05",
        }
    ]
    snapshot = store.snapshot()
    assert snapshot["slots"]["C05"]["state"] == SLOT_OCCUPIED
    assert snapshot["slots"]["C05"]["item_id"] == "item-5"
    assert snapshot["slots"]["C06"]["state"] == SLOT_AVAILABLE
    assert snapshot["items"]["item-5"]["label"] == "C05"


def test_cloud_delete_releases_local_slot_and_blocks_stale_reimport(tmp_path):
    store = inventory(tmp_path)
    store.reserve_store("store-5", 11, "C05")
    store.bind_active_store_item("item-5")
    store.apply_task_status("store-5", 11, "done", "C05")

    result = store.mark_cloud_items_deleted(["item-5"])

    assert result["released_slots"] == [{"item_id": "item-5", "slot": "C05"}]
    assert result["cancelled_cloud_updates"] == [{"command_id": "store-5", "item_id": "item-5"}]
    assert store.pending_cloud_updates() == []
    snapshot = store.snapshot()
    assert snapshot["slots"]["C05"]["state"] == SLOT_AVAILABLE
    assert snapshot["items"]["item-5"]["state"] == "deleted"
    assert "label" not in snapshot["items"]["item-5"]

    store.reconcile_cloud_items([{"id": "item-5", "status": "in-wardrobe", "real_position": "C05"}])
    cloud_item = {"id": "item-5", "status": "in-wardrobe", "real_position": "C05"}
    store.annotate_items([cloud_item])
    assert store.snapshot()["slots"]["C05"]["state"] == SLOT_AVAILABLE
    assert cloud_item["status"] == "deleted"


def test_cloud_delete_releases_both_slots_from_legacy_duplicate_mapping(tmp_path):
    path = tmp_path / "store_positions.json"
    store = WardrobeInventory(path)
    store.reconcile_cloud_items([{"id": "item-5", "status": "in-wardrobe", "real_position": "C05"}])
    damaged = store.snapshot()
    damaged["slots"]["C06"].update(state=SLOT_OCCUPIED, item_id="item-5")
    path.write_text(json.dumps(damaged), encoding="utf-8")
    restarted = WardrobeInventory(path)

    result = restarted.mark_cloud_items_deleted(["item-5"])

    assert result["released_slots"] == [
        {"item_id": "item-5", "slot": "C05"},
        {"item_id": "item-5", "slot": "C06"},
    ]
    snapshot = restarted.snapshot()
    assert snapshot["slots"]["C05"]["state"] == SLOT_AVAILABLE
    assert snapshot["slots"]["C06"]["state"] == SLOT_AVAILABLE


def test_cloud_delete_rejects_item_in_active_fetch_transaction(tmp_path):
    store = inventory(tmp_path)
    store.reconcile_cloud_items([{"id": "item-5", "status": "in-wardrobe", "real_position": "C05"}])
    store.reserve_fetch("fetch-5", 7, item_id="item-5")

    try:
        store.validate_cloud_delete(["item-5"])
    except InventoryError as exc:
        assert "active fetch transaction" in str(exc)
    else:
        raise AssertionError("an item in an active fetch transaction must not be deleted")


def test_fetch_uses_item_slot_and_releases_only_after_done(tmp_path):
    store = inventory(tmp_path)
    store.reserve_store("store-1", 11, "C05")
    store.bind_active_store_item("item-5")
    store.apply_task_status("store-1", 11, "done", "C05")
    store.mark_cloud_sync("store-1", True)

    tx = store.reserve_fetch("fetch-1", 7, item_id="item-5")
    assert tx["slot"] == "C05"
    assert store.snapshot()["slots"]["C05"]["state"] == SLOT_RESERVED_FETCH

    store.apply_task_status("fetch-1", 7, "done", "C05")
    snapshot = store.snapshot()
    assert snapshot["slots"]["C05"]["state"] == SLOT_AVAILABLE
    assert snapshot["items"]["item-5"]["state"] == "in-use"
    assert store.pending_cloud_updates() == [
        {
            "command_id": "fetch-1",
            "item_id": "item-5",
            "status": "in-use",
            "real_position": "C05",
            "real_position_index": 4,
        }
    ]


def test_error_slot_is_not_silently_reused(tmp_path):
    store = inventory(tmp_path)
    store.reserve_store("store-1", 11, "C01")
    store.apply_task_status("store-1", 11, "error", "C01")

    assert store.snapshot()["slots"]["C01"]["state"] == SLOT_ERROR
    assert store.reserve_store("store-2", 11)["slot"] == "C02"


def test_fetch_rejects_wrong_target_for_item(tmp_path):
    store = inventory(tmp_path)
    store.reserve_store("store-1", 11, "C03")
    store.bind_active_store_item("item-3")
    store.apply_task_status("store-1", 11, "done", "C03")

    try:
        store.reserve_fetch("fetch-1", 7, item_id="item-3", target_c="C04")
    except InventoryError as exc:
        assert "belongs to C03" in str(exc)
    else:
        raise AssertionError("wrong target must be rejected")


def test_send_failure_rolls_back_unstarted_reservation(tmp_path):
    store = inventory(tmp_path)
    store.reserve_store("store-1", 11, "C07")
    store.cancel_dispatch("store-1", "serial closed")
    assert store.snapshot()["slots"]["C07"]["state"] == SLOT_AVAILABLE


def test_reset_all_available_restarts_allocation_at_c01(tmp_path):
    store = inventory(tmp_path)
    store.reserve_store("store-error-c01", 11, "C01")
    store.mark_dispatched("store-error-c01")
    store.apply_task_status("store-error-c01", 11, "error", "C01")
    store.reserve_store("store-await-c02", 11, "C02")
    store.mark_dispatched("store-await-c02")
    store.apply_task_status("store-await-c02", 11, "done", "C02")

    result = store.reset_all_available("operator requested clean start")

    snapshot = store.snapshot()
    assert result["next_store_slot"] == "C01"
    assert all(slot["state"] == SLOT_AVAILABLE for slot in snapshot["slots"].values())
    next_transaction = store.reserve_store("store-next", 11)
    assert next_transaction["slot"] == "C01"


def test_reset_all_available_rejects_active_transaction(tmp_path):
    store = inventory(tmp_path)
    store.reserve_store("store-active", 11, "C01")

    try:
        store.reset_all_available()
    except InventoryError as exc:
        assert "is active" in str(exc)
    else:
        raise AssertionError("active inventory transaction must block reset")
