from datetime import datetime

from wardrobe_gateway.care_schedule import CareScheduleStore, CareScheduler, is_quiet_time, normalize_schedule


def test_normalize_schedule_keeps_known_devices():
    schedule = normalize_schedule(
        {
            "id": "s1",
            "name": "morning",
            "time": "7:5",
            "duration_minutes": 3,
            "devices": ["hot-air", "uv_lamp", "vent_fan", "ion"],
            "weekdays": [0, 6, 9],
        }
    )

    assert schedule.time == "07:05"
    assert schedule.times == ["07:05"]
    assert schedule.duration_minutes == 3
    assert schedule.devices == ["hot_air", "uv", "vent", "ionizer"]
    assert schedule.weekdays == [0, 6]


def test_normalize_schedule_keeps_multiple_time_points():
    schedule = normalize_schedule(
        {
            "id": "multi",
            "times": ["7:5", "12:30", "07:05", "24:99"],
            "devices": ["uv"],
        }
    )

    assert schedule.time == "07:05"
    assert schedule.times == ["07:05", "12:30", "23:59"]


def test_scheduler_runs_once_and_skips_reserved_ionizer(tmp_path):
    sent = []

    def send_command(payload, source):
        sent.append((payload, source))
        return {"ok": True}

    store = CareScheduleStore(tmp_path / "care_schedules.json")
    store.update_one(
        {
            "id": "daily-care",
            "name": "daily",
            "enabled": True,
            "times": ["09:30", "21:00"],
            "duration_minutes": 1,
            "devices": ["hot_air", "uv", "vent", "ionizer"],
            "weekdays": [0],
        }
    )
    scheduler = CareScheduler(store, send_command, interval_seconds=999)

    now = datetime(2026, 6, 1, 9, 30)
    scheduler.tick(now=now, now_epoch=1000)
    scheduler.tick(now=now, now_epoch=1010)
    scheduler.tick(now=datetime(2026, 6, 1, 21, 0), now_epoch=2000)

    assert [item[0]["care_device"] for item in sent] == ["hot_air", "uv", "vent", "hot_air", "uv", "vent"]
    assert [item[0]["care_phase"] for item in sent] == ["start", "start", "start", "start", "start", "start"]
    assert all(source == "care-schedule" for _, source in sent)


def test_scheduler_stops_started_devices_after_duration(tmp_path):
    sent = []

    def send_command(payload, source):
        sent.append(payload)
        return {"ok": True}

    store = CareScheduleStore(tmp_path / "care_schedules.json")
    store.update_one(
        {
            "id": "daily-care",
            "enabled": True,
            "time": "09:30",
            "duration_minutes": 1,
            "devices": ["uv", "vent"],
            "weekdays": [0],
        }
    )
    scheduler = CareScheduler(store, send_command, interval_seconds=999)

    scheduler.tick(now=datetime(2026, 6, 1, 9, 30), now_epoch=1000)
    scheduler.tick(now=datetime(2026, 6, 1, 9, 31), now_epoch=1061)

    assert [item["care_device"] for item in sent] == ["uv", "vent", "uv", "vent"]
    assert [item["care_phase"] for item in sent] == ["start", "start", "stop", "stop"]


def test_advanced_schedule_normalizes_runtime_controls():
    schedule = normalize_schedule(
        {
            "id": "advanced",
            "stagger_seconds": 999,
            "min_interval_minutes": 2000,
            "quiet_start": "22:5",
            "quiet_end": "6:30",
            "recent_events": [{"event": "started", "at": "12", "devices": ["hot-air"]}],
        }
    )

    assert schedule.stagger_seconds == 300
    assert schedule.min_interval_minutes == 1440
    assert schedule.quiet_start == "22:05"
    assert schedule.quiet_end == "06:30"
    assert schedule.recent_events == [{"event": "started", "reason": "", "at": 12.0, "devices": ["hot_air"]}]
    assert is_quiet_time("23:00", schedule.quiet_start, schedule.quiet_end)
    assert is_quiet_time("05:59", schedule.quiet_start, schedule.quiet_end)
    assert not is_quiet_time("12:00", schedule.quiet_start, schedule.quiet_end)


def test_scheduler_skips_quiet_min_interval_and_overlap(tmp_path):
    sent = []

    def send_command(payload, source):
        sent.append(payload)
        return {"ok": True}

    store = CareScheduleStore(tmp_path / "care_schedules.json")
    store.update_one(
        {
            "id": "advanced",
            "enabled": True,
            "times": ["09:00", "09:01", "09:02", "23:00"],
            "duration_minutes": 10,
            "devices": ["uv"],
            "weekdays": [0],
            "min_interval_minutes": 30,
            "quiet_start": "22:00",
            "quiet_end": "06:00",
        }
    )
    scheduler = CareScheduler(store, send_command, interval_seconds=999)

    scheduler.tick(now=datetime(2026, 6, 1, 9, 0), now_epoch=1000)
    scheduler.tick(now=datetime(2026, 6, 1, 9, 1), now_epoch=1060)
    scheduler.tick(now=datetime(2026, 6, 1, 9, 2), now_epoch=1120)
    scheduler.tick(now=datetime(2026, 6, 1, 9, 11), now_epoch=1661)
    scheduler.tick(now=datetime(2026, 6, 1, 23, 0), now_epoch=2000)

    assert [item["care_phase"] for item in sent] == ["start", "stop"]
    saved = store.list()[0]
    assert saved["last_result"] == "skipped"
    assert [event["reason"] for event in saved["recent_events"]] == ["", "already_running", "already_running", "", "quiet_window"]


def test_scheduler_staggers_started_devices(tmp_path):
    sleeps = []
    sent = []

    def send_command(payload, source):
        sent.append(payload)
        return {"ok": True}

    store = CareScheduleStore(tmp_path / "care_schedules.json")
    store.update_one(
        {
            "id": "stagger",
            "duration_minutes": 1,
            "devices": ["hot_air", "uv", "vent"],
            "stagger_seconds": 4,
        }
    )
    item = normalize_schedule(store.list()[0])
    scheduler = CareScheduler(store, send_command, interval_seconds=999, sleep=sleeps.append)
    scheduler.run_schedule(item, now_epoch=1000)

    assert sleeps == [4, 4]
    assert [item["care_sequence_index"] for item in sent] == [0, 1, 2]
    assert [item["care_stagger_seconds"] for item in sent] == [4, 4, 4]
    saved = store.list()[0]
    assert saved["last_run_epoch"] == 1008
    assert saved["running_until"] == 1068
