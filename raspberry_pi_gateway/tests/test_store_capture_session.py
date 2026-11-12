import os
import tempfile
from pathlib import Path

import pytest

os.environ.setdefault(
    "WARDROBE_CAPTURE_DIR",
    str(Path(tempfile.gettempdir()) / "wardrobe_gateway_tests" / "captures"),
)

from wardrobe_gateway import app as gateway_app
from wardrobe_gateway.inventory import InventoryError, WardrobeInventory


def test_cloud_display_page_disables_browser_cache():
    response = gateway_app.cloud_display_page()

    assert response.headers["cache-control"] == "no-store, max-age=0"
    assert response.headers["pragma"] == "no-cache"


def reset_store_capture_session() -> None:
    with gateway_app.store_capture_session_lock:
        gateway_app.store_capture_session.update(
            {
                "id": "",
                "started_at": 0.0,
                "captures": 0,
                "front_pending": False,
                "front_capture": None,
                "paired_uploaded": False,
            }
        )


def test_board_front_single_capture_uploads_once_and_binds_active_slot(tmp_path, monkeypatch):
    store = WardrobeInventory(tmp_path / "inventory.json")
    store.reserve_store("store-single-3", 11, "C03")
    store.mark_dispatched("store-single-3")
    monkeypatch.setattr(gateway_app, "inventory", store)
    image_path = tmp_path / "front.jpg"
    image_path.write_bytes(b"front")
    capture_calls = []
    upload_calls = []
    monkeypatch.setattr(gateway_app.camera_uploader, "capture", lambda: capture_calls.append(True) or image_path)

    def fake_upload(path, *, cloud_metadata=None):
        upload_calls.append((path, cloud_metadata))
        return 0, 1, {
            "ok": True,
            "response": {"item": {"id": "item-single-3", "style": "T恤"}},
            "cloud_metadata": cloud_metadata,
        }

    monkeypatch.setattr(gateway_app.camera_uploader, "upload", fake_upload)
    monkeypatch.setattr(gateway_app, "_post_cloud_item_status", lambda *args, **kwargs: None)

    status, style_id, result, upload_to_cloud, source = gateway_app._capture_board_store_image(0)

    assert (status, style_id, upload_to_cloud, source) == (0, 1, True, "board1-front-single")
    assert result["capture_mode"] == "single-front"
    assert len(capture_calls) == 1
    assert len(upload_calls) == 1
    metadata = upload_calls[0][1]
    assert metadata["capture_session_id"] == "store-single-3"
    assert metadata["real_position"] == "C03"
    snapshot = store.snapshot()
    assert snapshot["transactions"]["store-single-3"]["item_id"] == "item-single-3"
    assert snapshot["items"]["item-single-3"]["label"] == "C03"


def test_board_back_is_explicitly_skipped_without_capture_or_upload(monkeypatch):
    monkeypatch.setattr(gateway_app.camera_uploader, "capture", lambda: pytest.fail("back trigger must not capture"))
    monkeypatch.setattr(gateway_app.camera_uploader, "upload", lambda *args, **kwargs: pytest.fail("back trigger must not upload"))

    status, style_id, result, upload_to_cloud, source = gateway_app._capture_board_store_image(1)

    assert (status, style_id, upload_to_cloud) == (0, 0, False)
    assert source == "board1-back-skipped-single-front"
    assert result["skipped"] is True
    assert result["capture_mode"] == "single-front"


def test_manual_capture_defaults_to_local_only_and_cloud_upload_requires_matching_transaction(tmp_path, monkeypatch):
    store = WardrobeInventory(tmp_path / "inventory.json")
    monkeypatch.setattr(gateway_app, "inventory", store)

    assert gateway_app._should_upload_capture({}) is False
    assert gateway_app._should_upload_capture({"upload": False}) is False
    assert gateway_app._capture_cloud_metadata({}, "manual") is None

    with pytest.raises(InventoryError, match="active store inventory transaction"):
        gateway_app._capture_cloud_metadata({"upload": True, "inventory_command_id": "store-1"}, "manual")

    store.reserve_store("store-1", 11, "C03")
    with pytest.raises(InventoryError, match="requires inventory_command_id"):
        gateway_app._capture_cloud_metadata({"upload": True}, "manual")
    with pytest.raises(InventoryError, match="does not match"):
        gateway_app._capture_cloud_metadata(
            {"upload": True, "inventory_command_id": "store-other"},
            "manual",
        )

    metadata = gateway_app._capture_cloud_metadata(
        {"upload": True, "inventory_command_id": "store-1"},
        "manual",
    )
    assert metadata["real_position"] == "C03"
    assert metadata["real_position_index"] == 2
    assert metadata["capture_session_id"] == "store-1"


def test_board_pair_requires_three_front_and_three_back_frames(tmp_path, monkeypatch):
    reset_store_capture_session()
    front_frames = []
    back_frames = []
    for index in range(3):
        path = tmp_path / f"front-{index}.jpg"
        path.write_bytes(b"front")
        front_frames.append(path)
    for index in range(2):
        path = tmp_path / f"back-{index}.jpg"
        path.write_bytes(b"back")
        back_frames.append(path)
    with gateway_app.store_capture_session_lock:
        gateway_app.store_capture_session["front_capture"] = {
            "multi_angle": {"frames": [str(path) for path in front_frames]}
        }
    monkeypatch.setattr(
        gateway_app.camera_uploader,
        "compose_vertical",
        lambda *args, **kwargs: pytest.fail("incomplete six-frame input must not be composed"),
    )

    status, _, result = gateway_app._upload_board_capture_pair(
        {"multi_angle": {"frames": [str(path) for path in back_frames]}},
        "board1-back",
    )

    assert status == gateway_app.STATUS_CAM_FAIL
    assert result["ok"] is False
    assert result["paired_multi_angle"]["expected_front_frames"] == 3
    assert len(result["paired_multi_angle"]["back_frames"]) == 2


def test_cloud_clear_helper_updates_cloud_then_releases_local_slot(tmp_path, monkeypatch):
    store = WardrobeInventory(tmp_path / "inventory.json")
    store.reconcile_cloud_items([{"id": "item-4", "status": "in-wardrobe", "real_position": "C04"}])
    monkeypatch.setattr(gateway_app, "inventory", store)
    cloud_calls = []
    monkeypatch.setattr(
        gateway_app,
        "_post_cloud_item_status",
        lambda item_id, status: cloud_calls.append((item_id, status)),
    )

    result = gateway_app._clear_cloud_wardrobe_items(["item-4"])

    assert result["ok"] is True
    assert cloud_calls == [("item-4", "deleted")]
    assert result["released_slots"] == [{"item_id": "item-4", "slot": "C04"}]
    snapshot = store.snapshot()
    assert snapshot["slots"]["C04"]["state"] == "available"
    assert snapshot["items"]["item-4"]["state"] == "deleted"


def test_cloud_item_on_local_error_slot_is_marked_unavailable_for_fetch(tmp_path, monkeypatch):
    store = WardrobeInventory(tmp_path / "inventory.json")
    store.reserve_store("store-error-3", 11, "C03")
    store.mark_dispatched("store-error-3")
    store.apply_task_status("store-error-3", 11, "error", "C03")
    monkeypatch.setattr(gateway_app, "inventory", store)
    items = [
        {
            "id": "item-cloud-3",
            "name": "云端衣物",
            "status": "in-wardrobe",
            "real_position": "C03",
            "image_status": "processing",
        }
    ]

    gateway_app._annotate_wardrobe_items(items)

    assert items[0]["status"] == "inventory-error"
    assert items[0]["inventory_conflict"]["slot"] == "C03"
    assert items[0]["inventory_conflict"]["local_state"] == "error"
