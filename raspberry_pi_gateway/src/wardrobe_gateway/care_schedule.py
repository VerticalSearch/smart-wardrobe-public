from __future__ import annotations

import json
import threading
import time
import uuid
from dataclasses import dataclass, field, replace
from datetime import datetime
from pathlib import Path
from typing import Any, Callable


CommandSender = Callable[[dict[str, Any], str], dict[str, Any]]
Sleeper = Callable[[float], None]

CARE_DEVICE_DEFS: dict[str, dict[str, Any]] = {
    "hot_air": {
        "label": "热风除湿",
        "start": {"action": "todo", "action_id": 8},
        "stop": {"action": "todo", "action_id": 9},
    },
    "ionizer": {
        "label": "负离子发生器",
        "supported": False,
        "note": "当前主板1固件未发现独立负离子输出，先作为计划预留项。",
    },
    "uv": {
        "label": "紫外线杀菌",
        "start": {"action": "board1_output", "device": "uv_lamp", "enable": 1},
        "stop": {"action": "board1_output", "device": "uv_lamp", "enable": 0},
    },
    "vent": {
        "label": "通风风扇",
        "start": {"action": "board1_output", "device": "vent_fan", "enable": 1},
        "stop": {"action": "board1_output", "device": "vent_fan", "enable": 0},
    },
}


@dataclass
class CareSchedule:
    id: str
    name: str
    enabled: bool
    time: str
    times: list[str]
    duration_minutes: int
    devices: list[str]
    weekdays: list[int]
    stagger_seconds: int = 0
    min_interval_minutes: int = 0
    quiet_start: str = ""
    quiet_end: str = ""
    last_start_key: str = ""
    last_run_epoch: float = 0.0
    last_stop_epoch: float = 0.0
    last_result: str = ""
    last_skip_reason: str = ""
    run_count: int = 0
    running_until: float = 0.0
    running_devices: list[str] = field(default_factory=list)
    recent_events: list[dict[str, Any]] = field(default_factory=list)

    def public_dict(self) -> dict[str, Any]:
        return {
            "id": self.id,
            "name": self.name,
            "enabled": self.enabled,
            "time": self.time,
            "times": self.times,
            "duration_minutes": self.duration_minutes,
            "devices": self.devices,
            "weekdays": self.weekdays,
            "stagger_seconds": self.stagger_seconds,
            "min_interval_minutes": self.min_interval_minutes,
            "quiet_start": self.quiet_start,
            "quiet_end": self.quiet_end,
            "last_start_key": self.last_start_key,
            "last_run_epoch": self.last_run_epoch,
            "last_stop_epoch": self.last_stop_epoch,
            "last_result": self.last_result,
            "last_skip_reason": self.last_skip_reason,
            "run_count": self.run_count,
            "running_until": self.running_until,
            "running_devices": self.running_devices,
            "recent_events": self.recent_events[-8:],
        }


class CareScheduleStore:
    def __init__(self, path: Path) -> None:
        self.path = path
        self._lock = threading.Lock()
        self._items: list[CareSchedule] = []
        self.load()

    def load(self) -> None:
        with self._lock:
            self._items = []
            if not self.path.exists():
                return
            try:
                data = json.loads(self.path.read_text(encoding="utf-8"))
            except (OSError, json.JSONDecodeError):
                return
            self._items = [normalize_schedule(item) for item in data.get("schedules", [])]

    def save(self) -> None:
        with self._lock:
            self._save_locked()

    def _save_locked(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        data = {"schedules": [item.public_dict() for item in self._items]}
        self.path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")

    def list(self) -> list[dict[str, Any]]:
        with self._lock:
            return [item.public_dict() for item in self._items]

    def replace(self, items: list[dict[str, Any]]) -> list[dict[str, Any]]:
        normalized = [normalize_schedule(item) for item in items]
        with self._lock:
            self._items = normalized
            self._save_locked()
            return [item.public_dict() for item in self._items]

    def update_one(self, payload: dict[str, Any]) -> dict[str, Any]:
        item = normalize_schedule(payload)
        with self._lock:
            for index, current in enumerate(self._items):
                if current.id == item.id:
                    merged = current.public_dict()
                    merged.update(payload)
                    item = normalize_schedule(merged)
                    self._items[index] = item
                    break
            else:
                self._items.append(item)
            self._save_locked()
            return item.public_dict()

    def delete(self, schedule_id: str) -> bool:
        with self._lock:
            before = len(self._items)
            self._items = [item for item in self._items if item.id != schedule_id]
            changed = len(self._items) != before
            if changed:
                self._save_locked()
            return changed

    def due_items(self, now: datetime, now_epoch: float | None = None) -> list[CareSchedule]:
        weekday = now.weekday()
        current_time = now.strftime("%H:%M")
        minute_key = now.strftime("%Y-%m-%dT") + current_time
        current_epoch = time.time() if now_epoch is None else now_epoch
        due: list[CareSchedule] = []
        with self._lock:
            changed = False
            for item in self._items:
                if not item.enabled:
                    continue
                if current_time not in item.times:
                    continue
                if item.weekdays and weekday not in item.weekdays:
                    continue
                if item.last_start_key == minute_key:
                    continue
                item.last_start_key = minute_key
                changed = True
                if item.running_until and current_epoch < item.running_until:
                    self._record_event_locked(item, "skipped", "already_running", current_epoch, [])
                    continue
                if is_quiet_time(current_time, item.quiet_start, item.quiet_end):
                    self._record_event_locked(item, "skipped", "quiet_window", current_epoch, [])
                    continue
                if item.min_interval_minutes > 0 and item.last_run_epoch > 0:
                    elapsed = current_epoch - item.last_run_epoch
                    if elapsed < item.min_interval_minutes * 60:
                        self._record_event_locked(item, "skipped", "min_interval", current_epoch, [])
                        continue
                due.append(item)
            if changed:
                self._save_locked()
        return due

    def mark_running(self, schedule_id: str, until_epoch: float, devices: list[str], now_epoch: float | None = None) -> None:
        with self._lock:
            for item in self._items:
                if item.id == schedule_id:
                    current_epoch = time.time() if now_epoch is None else now_epoch
                    item.running_until = until_epoch
                    item.running_devices = list(devices)
                    item.last_run_epoch = current_epoch
                    item.last_result = "running"
                    item.last_skip_reason = ""
                    item.run_count += 1
                    self._record_event_locked(item, "started", "", current_epoch, devices)
                    self._save_locked()
                    return

    def pop_expired_runs(self, now_epoch: float) -> list[CareSchedule]:
        expired: list[CareSchedule] = []
        with self._lock:
            for item in self._items:
                if item.running_until and now_epoch >= item.running_until:
                    expired.append(replace(item, running_devices=list(item.running_devices)))
                    item.running_until = 0.0
                    item.running_devices = []
            if expired:
                self._save_locked()
        return expired

    def mark_stopped(self, schedule_id: str, devices: list[str], now_epoch: float | None = None) -> None:
        with self._lock:
            for item in self._items:
                if item.id == schedule_id:
                    current_epoch = time.time() if now_epoch is None else now_epoch
                    item.last_stop_epoch = current_epoch
                    item.last_result = "stopped"
                    self._record_event_locked(item, "stopped", "", current_epoch, devices)
                    self._save_locked()
                    return

    def _record_event_locked(
        self,
        item: CareSchedule,
        event: str,
        reason: str,
        at_epoch: float,
        devices: list[str],
    ) -> None:
        item.last_skip_reason = reason if event == "skipped" else ""
        if event == "skipped":
            item.last_result = "skipped"
        item.recent_events.append(
            {
                "event": event,
                "reason": reason,
                "at": at_epoch,
                "devices": list(devices),
            }
        )
        item.recent_events = item.recent_events[-8:]


class CareScheduler:
    def __init__(
        self,
        store: CareScheduleStore,
        send_command: CommandSender,
        interval_seconds: float = 30.0,
        sleep: Sleeper = time.sleep,
    ) -> None:
        self.store = store
        self.send_command = send_command
        self.interval_seconds = interval_seconds
        self.sleep = sleep
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None

    def start(self) -> None:
        if self._thread and self._thread.is_alive():
            return
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, name="care-scheduler", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self._thread:
            self._thread.join(timeout=2)

    def _run(self) -> None:
        while not self._stop.wait(self.interval_seconds):
            self.tick()

    def tick(self, now: datetime | None = None, now_epoch: float | None = None) -> None:
        current = now or datetime.now()
        current_epoch = time.time() if now_epoch is None else now_epoch
        for item in self.store.due_items(current, current_epoch):
            self.run_schedule(item, current_epoch)
        for item in self.store.pop_expired_runs(current_epoch):
            self.stop_schedule(item, current_epoch)

    def run_schedule(self, item: CareSchedule, now_epoch: float | None = None) -> None:
        started_devices = self._send_device_commands(item, "start")
        if not started_devices:
            return
        duration_seconds = max(1, item.duration_minutes) * 60
        if now_epoch is None:
            current_epoch = time.time()
        else:
            current_epoch = now_epoch + max(0, len(started_devices) - 1) * item.stagger_seconds
        self.store.mark_running(item.id, current_epoch + duration_seconds, started_devices, current_epoch)

    def stop_schedule(self, item: CareSchedule, now_epoch: float | None = None) -> None:
        devices = item.running_devices or item.devices
        stopped_devices = self._send_device_commands(item, "stop", devices=devices)
        if stopped_devices:
            self.store.mark_stopped(item.id, stopped_devices, now_epoch)

    def run_manual(self, payload: dict[str, Any]) -> dict[str, Any]:
        item = normalize_schedule(self.store.update_one(payload))
        self.run_schedule(item)
        for schedule in self.store.list():
            if schedule.get("id") == item.id:
                return schedule
        return item.public_dict()

    def _send_device_commands(self, item: CareSchedule, phase: str, devices: list[str] | None = None) -> list[str]:
        sent: list[str] = []
        sequence_index = 0
        for device in devices or item.devices:
            definition = CARE_DEVICE_DEFS.get(device)
            if not definition or not definition.get("supported", True):
                continue
            if phase == "start" and sent and item.stagger_seconds > 0:
                self.sleep(item.stagger_seconds)
            command = dict(definition[phase])
            command.update(
                {
                    "id": f"care-auto-{item.id}-{device}-{phase}-{int(time.time() * 1000)}",
                    "source": "care-schedule",
                    "schedule_id": item.id,
                    "schedule_name": item.name,
                    "care_device": device,
                    "care_phase": phase,
                    "care_sequence_index": sequence_index,
                    "care_stagger_seconds": item.stagger_seconds if phase == "start" else 0,
                }
            )
            result = self.send_command(command, "care-schedule")
            if result.get("ok"):
                sent.append(device)
                sequence_index += 1
        return sent


def normalize_schedule(payload: dict[str, Any]) -> CareSchedule:
    if not isinstance(payload, dict):
        raise ValueError("schedule must be a JSON object")
    schedule_id = str(payload.get("id") or f"care-{uuid.uuid4().hex[:8]}")
    name = str(payload.get("name") or "自动养护").strip()[:32] or "自动养护"
    times = normalize_times(payload.get("times", payload.get("time", "09:00")))
    time_text = times[0]
    duration = clamp_int(payload.get("duration_minutes", payload.get("duration", 15)), 15, 1, 180)
    devices = normalize_devices(payload.get("devices", ["hot_air", "ionizer", "uv"]))
    weekdays = normalize_weekdays(payload.get("weekdays", []))
    quiet_start = normalize_optional_time(payload.get("quiet_start", ""))
    quiet_end = normalize_optional_time(payload.get("quiet_end", ""))
    return CareSchedule(
        id=schedule_id,
        name=name,
        enabled=bool(payload.get("enabled", True)),
        time=time_text,
        times=times,
        duration_minutes=duration,
        devices=devices,
        weekdays=weekdays,
        stagger_seconds=clamp_int(payload.get("stagger_seconds", 0), 0, 0, 300),
        min_interval_minutes=clamp_int(payload.get("min_interval_minutes", 0), 0, 0, 1440),
        quiet_start=quiet_start,
        quiet_end=quiet_end,
        last_start_key=str(payload.get("last_start_key") or ""),
        last_run_epoch=safe_float(payload.get("last_run_epoch"), 0.0),
        last_stop_epoch=safe_float(payload.get("last_stop_epoch"), 0.0),
        last_result=str(payload.get("last_result") or ""),
        last_skip_reason=str(payload.get("last_skip_reason") or ""),
        run_count=clamp_int(payload.get("run_count", 0), 0, 0, 999999),
        running_until=safe_float(payload.get("running_until"), 0.0),
        running_devices=normalize_devices(payload.get("running_devices", []), allow_empty=True),
        recent_events=normalize_recent_events(payload.get("recent_events", [])),
    )


def normalize_time(value: Any) -> str:
    text = str(value or "").strip()
    parts = text.split(":")
    if len(parts) < 2:
        return "09:00"
    hour = clamp_int(parts[0], 9, 0, 23)
    minute = clamp_int(parts[1], 0, 0, 59)
    return f"{hour:02d}:{minute:02d}"


def normalize_optional_time(value: Any) -> str:
    text = str(value or "").strip()
    if not text:
        return ""
    return normalize_time(text)


def normalize_times(value: Any) -> list[str]:
    raw_items = value if isinstance(value, list) else [value]
    times: list[str] = []
    for raw in raw_items:
        time_text = normalize_time(raw)
        if time_text not in times:
            times.append(time_text)
    return times or ["09:00"]


def normalize_devices(value: Any, allow_empty: bool = False) -> list[str]:
    aliases = {
        "hot-air": "hot_air",
        "hotair": "hot_air",
        "dehumidify": "hot_air",
        "uv_lamp": "uv",
        "ultraviolet": "uv",
        "negative_ion": "ionizer",
        "ion": "ionizer",
        "fan": "vent",
        "vent_fan": "vent",
    }
    raw_items = value if isinstance(value, list) else [value]
    devices: list[str] = []
    for raw in raw_items:
        key = aliases.get(str(raw or "").strip().lower(), str(raw or "").strip().lower())
        if key in CARE_DEVICE_DEFS and key not in devices:
            devices.append(key)
    if devices or allow_empty:
        return devices
    return ["hot_air"]


def normalize_weekdays(value: Any) -> list[int]:
    raw_items = value if isinstance(value, list) else []
    days: list[int] = []
    for raw in raw_items:
        day = clamp_int(raw, -1, -1, 6)
        if day >= 0 and day not in days:
            days.append(day)
    return sorted(days)


def normalize_recent_events(value: Any) -> list[dict[str, Any]]:
    if not isinstance(value, list):
        return []
    events: list[dict[str, Any]] = []
    for raw in value[-8:]:
        if not isinstance(raw, dict):
            continue
        events.append(
            {
                "event": str(raw.get("event") or "")[:24],
                "reason": str(raw.get("reason") or "")[:32],
                "at": safe_float(raw.get("at"), 0.0),
                "devices": normalize_devices(raw.get("devices", []), allow_empty=True),
            }
        )
    return events


def time_to_minutes(value: str) -> int:
    hour, minute = normalize_time(value).split(":")
    return int(hour) * 60 + int(minute)


def is_quiet_time(current: str, quiet_start: str, quiet_end: str) -> bool:
    if not quiet_start or not quiet_end or quiet_start == quiet_end:
        return False
    current_minutes = time_to_minutes(current)
    start_minutes = time_to_minutes(quiet_start)
    end_minutes = time_to_minutes(quiet_end)
    if start_minutes < end_minutes:
        return start_minutes <= current_minutes < end_minutes
    return current_minutes >= start_minutes or current_minutes < end_minutes


def safe_float(value: Any, default: float) -> float:
    try:
        return float(value)
    except (TypeError, ValueError, OverflowError):
        return default


def clamp_int(value: Any, default: int, min_value: int, max_value: int) -> int:
    try:
        number = int(value)
    except (TypeError, ValueError, OverflowError):
        number = default
    return max(min_value, min(max_value, number))
