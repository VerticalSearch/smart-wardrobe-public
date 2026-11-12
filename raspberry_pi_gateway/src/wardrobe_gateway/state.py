from __future__ import annotations

import threading
from collections import deque
from datetime import datetime, timezone
from typing import Any


class GatewayState:
    def __init__(self, max_logs: int = 240) -> None:
        self._lock = threading.Lock()
        self._logs: deque[dict[str, Any]] = deque(maxlen=max_logs)
        self.serial_connected = False
        self.serial_port = ""
        self.serial_error = ""
        self.camera_ok = False
        self.camera_error = ""
        self.last_capture: dict[str, Any] | None = None
        self.last_upload: dict[str, Any] | None = None
        self.last_command: dict[str, Any] | None = None
        self.last_task_event: dict[str, Any] | None = None
        self.task_events: deque[dict[str, Any]] = deque(maxlen=20)
        self.mqtt_connected = False
        self.mqtt_error = ""
        self.care_gravity_left: deque[float] = deque(maxlen=48)
        self.care_gravity_right: deque[float] = deque(maxlen=48)
        self.care_gravity_updated_at: str | None = None
        self.care_gravity_unit = "counts"
        self.started_at = self._now()

    @staticmethod
    def _now() -> str:
        return datetime.now(timezone.utc).isoformat()

    def add_log(self, direction: str, message: str, meta: dict[str, Any] | None = None) -> None:
        entry = {
            "time": self._now(),
            "direction": direction,
            "message": message,
            "meta": meta or {},
        }
        with self._lock:
            self._logs.appendleft(entry)

    def logs(self, limit: int = 100) -> list[dict[str, Any]]:
        with self._lock:
            return list(self._logs)[: max(0, min(limit, len(self._logs)))]

    def add_care_gravity_sample(
        self,
        left: float | None = None,
        right: float | None = None,
        source: str = "serial",
        unit: str = "counts",
    ) -> dict[str, Any]:
        now = self._now()
        with self._lock:
            changed = False
            if left is not None:
                self.care_gravity_left.append(float(left))
                changed = True
            if right is not None:
                self.care_gravity_right.append(float(right))
                changed = True
            if changed:
                self.care_gravity_updated_at = now
                self.care_gravity_unit = unit
            return self._care_gravity_snapshot_locked(source=source, unit=unit)

    def add_task_event(
        self,
        task_type: str = "",
        action_id: int | None = None,
        command_id: str = "",
        state: str = "",
        source: str = "serial",
        detail: str = "",
    ) -> dict[str, Any]:
        event = {
            "time": self._now(),
            "type": task_type,
            "action_id": action_id,
            "id": command_id,
            "state": state,
            "source": source,
            "detail": detail,
        }
        with self._lock:
            self.last_task_event = event
            self.task_events.appendleft(event)
        self.add_log("task", f"{task_type or '--'} {state or '--'}", {"id": command_id, "action_id": action_id, "source": source})
        return event

    def care_gravity_snapshot(self, source: str = "state", unit: str | None = None) -> dict[str, Any]:
        with self._lock:
            return self._care_gravity_snapshot_locked(source=source, unit=unit)

    def _care_gravity_snapshot_locked(self, source: str = "state", unit: str | None = None) -> dict[str, Any]:
        left = list(self.care_gravity_left)
        right = list(self.care_gravity_right)
        display_unit = unit or self.care_gravity_unit
        return {
            "ok": bool(left or right),
            "source": source,
            "unit": display_unit,
            "updated_at": self.care_gravity_updated_at,
            "left_value": left[-1] if left else None,
            "right_value": right[-1] if right else None,
            "left_connected": bool(left),
            "right_connected": bool(right),
            "series": {
                "left": left,
                "right": right,
            },
            "points": [left, right],
        }

    def snapshot(self) -> dict[str, Any]:
        with self._lock:
            return {
                "started_at": self.started_at,
                "serial": {
                    "connected": self.serial_connected,
                    "port": self.serial_port,
                    "error": self.serial_error,
                },
                "camera": {
                    "ok": self.camera_ok,
                    "error": self.camera_error,
                },
                "mqtt": {
                    "connected": self.mqtt_connected,
                    "error": self.mqtt_error,
                },
                "last_capture": self.last_capture,
                "last_upload": self.last_upload,
                "last_command": self.last_command,
                "last_task_event": self.last_task_event,
                "task_events": list(self.task_events),
                "care_gravity": self._care_gravity_snapshot_locked(),
                "logs": list(self._logs)[:20],
            }


gateway_state = GatewayState()
