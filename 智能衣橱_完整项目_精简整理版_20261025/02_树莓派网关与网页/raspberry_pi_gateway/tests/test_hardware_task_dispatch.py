import asyncio
import os
import tempfile
from pathlib import Path

import pytest
from fastapi import HTTPException

os.environ.setdefault(
    "WARDROBE_CAPTURE_DIR",
    str(Path(tempfile.gettempdir()) / "wardrobe_gateway_tests" / "captures"),
)

from wardrobe_gateway import app as gateway_app


class FakeResponse:
    def __init__(self, status_code, body):
        self.status_code = status_code
        self._body = body
        self.text = str(body)

    def json(self):
        return self._body


def test_hardware_task_dispatch_rolls_back_cloud_task_when_serial_fails(monkeypatch):
    calls = {"post": [], "delete": []}

    def fake_post(url, **kwargs):
        calls["post"].append((url, kwargs))
        return FakeResponse(200, {"id": "task-created-1"})

    def fake_delete(url, **kwargs):
        calls["delete"].append((url, kwargs))
        return FakeResponse(200, {"ok": True})

    monkeypatch.setattr(gateway_app.requests, "post", fake_post)
    monkeypatch.setattr(gateway_app.requests, "delete", fake_delete)
    monkeypatch.setattr(
        gateway_app,
        "_send_commands",
        lambda payload, source: {"ok": False, "detail": "sent=0, failed=1", "commands": [], "failed": ["@ACMD"]},
    )

    result = gateway_app._dispatch_hardware_task(
        {"type": "fetch", "item_ids": ["item-1"], "payload": {"source": "display"}},
        {"action": "todo", "action_id": 7, "id": "fetch-atomic-1", "item_id": "item-1", "target_c": "C01"},
    )

    assert result["taskCreated"] is True
    assert result["taskOk"] is False
    assert result["serialOk"] is False
    assert result["rollback"] == {"ok": True, "state": "deleted", "task_id": "task-created-1"}
    assert calls["post"][0][1]["json"]["payload"]["gateway_command_id"] == "fetch-atomic-1"
    assert calls["delete"][0][0].endswith("/api/tasks/task-created-1")


def test_hardware_task_dispatch_keeps_cloud_task_when_serial_succeeds(monkeypatch):
    monkeypatch.setattr(
        gateway_app.requests,
        "post",
        lambda *args, **kwargs: FakeResponse(200, {"task": {"id": "task-created-2"}}),
    )
    monkeypatch.setattr(
        gateway_app.requests,
        "delete",
        lambda *args, **kwargs: (_ for _ in ()).throw(AssertionError("successful dispatch must not roll back")),
    )
    monkeypatch.setattr(
        gateway_app,
        "_send_commands",
        lambda payload, source: {"ok": True, "detail": "sent=1, failed=0", "commands": ["@ACMD"], "failed": []},
    )

    result = gateway_app._dispatch_hardware_task(
        {"type": "store-new", "item_ids": [], "payload": {"source": "display"}},
        {"action": "todo", "action_id": 11, "id": "store-atomic-2"},
    )

    assert result["ok"] is True
    assert result["taskOk"] is True
    assert result["serialOk"] is True
    assert result["taskId"] == "task-created-2"
    assert result["rollback"] is None


@pytest.mark.parametrize("action_id", [6, 7, 11])
def test_direct_http_inventory_actions_are_rejected(monkeypatch, action_id):
    monkeypatch.setattr(
        gateway_app,
        "_send_commands",
        lambda *args, **kwargs: (_ for _ in ()).throw(AssertionError("direct inventory action must not dispatch")),
    )

    with pytest.raises(HTTPException) as exc_info:
        asyncio.run(gateway_app.control({"action": "todo", "action_id": action_id, "id": "legacy"}))

    assert exc_info.value.status_code == 409
    assert "/api/hardware/task-dispatch" in str(exc_info.value.detail)


def test_direct_http_non_inventory_action_still_dispatches(monkeypatch):
    monkeypatch.setattr(
        gateway_app,
        "_send_commands",
        lambda payload, source: {"ok": True, "detail": "sent=1, failed=0", "commands": ["@ACMD"], "failed": []},
    )
    monkeypatch.setattr(gateway_app, "mqtt_bridge", None)

    response = asyncio.run(
        gateway_app.control({"action": "todo", "action_id": 14, "id": "fetch-reset-test"})
    )

    assert response.status_code == 200


def test_legacy_direct_cloud_task_creation_is_rejected():
    with pytest.raises(HTTPException) as exc_info:
        asyncio.run(
            gateway_app.reject_legacy_task_create(
                {"type": "fetch", "item_ids": ["item-1"], "payload": {"source": "display"}}
            )
        )

    assert exc_info.value.status_code == 409
    assert "/api/hardware/task-dispatch" in str(exc_info.value.detail)
