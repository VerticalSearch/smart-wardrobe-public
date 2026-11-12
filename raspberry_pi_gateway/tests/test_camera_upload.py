from pathlib import Path
from types import SimpleNamespace

from wardrobe_gateway import camera as camera_module
from wardrobe_gateway.camera import CameraUploader
from wardrobe_gateway.state import GatewayState


class FakeResponse:
    status_code = 200
    text = '{"item":{"id":"item-3"}}'

    @staticmethod
    def json():
        return {"item": {"id": "item-3"}}


def test_positioned_upload_uses_multi_endpoint(tmp_path: Path, monkeypatch):
    image = tmp_path / "capture.jpg"
    image.write_bytes(b"jpeg-data")
    config = SimpleNamespace(
        capture_dir=tmp_path,
        upload_url="http://cloud/api/wardrobe/add",
        upload_multi_url="http://cloud/api/wardrobe/add-multi",
        device_token="",
    )
    uploader = CameraUploader(config, GatewayState())
    captured = {}

    def fake_post(url, json, headers, timeout):
        captured.update(url=url, json=json, headers=headers, timeout=timeout)
        return FakeResponse()

    monkeypatch.setattr(camera_module.requests, "post", fake_post)
    status, _, result = uploader.upload(
        image,
        {
            "capture_session_id": "store-3",
            "source": "board1-back",
            "real_position": "C03",
            "real_position_index": 2,
        },
    )

    assert status == 0
    assert result["item_id"] == "item-3"
    assert captured["url"].endswith("/api/wardrobe/add-multi")
    assert len(captured["json"]["images_base64"]) == 1
    assert captured["json"]["real_position"] == "C03"
    assert captured["json"]["real_position_index"] == 2


def test_unpositioned_upload_is_blocked_before_cloud_request(tmp_path: Path, monkeypatch):
    image = tmp_path / "capture.jpg"
    image.write_bytes(b"jpeg-data")
    config = SimpleNamespace(
        capture_dir=tmp_path,
        upload_url="http://cloud/api/wardrobe/add",
        upload_multi_url="http://cloud/api/wardrobe/add-multi",
        device_token="",
    )
    uploader = CameraUploader(config, GatewayState())
    monkeypatch.setattr(
        camera_module.requests,
        "post",
        lambda *args, **kwargs: (_ for _ in ()).throw(AssertionError("cloud request must be blocked")),
    )

    status, _, result = uploader.upload(image)

    assert status == camera_module.STATUS_UPLOAD_FAIL
    assert result["ok"] is False
    assert "requires real_position" in result["error"]


def test_compose_failure_returns_none_instead_of_single_frame_fallback(tmp_path: Path, monkeypatch):
    first = tmp_path / "first.jpg"
    second = tmp_path / "second.jpg"
    first.write_bytes(b"jpeg-1")
    second.write_bytes(b"jpeg-2")
    config = SimpleNamespace(capture_dir=tmp_path)
    uploader = CameraUploader(config, GatewayState())
    monkeypatch.setattr(uploader, "_run_command", lambda command: (False, "ffmpeg failed"))

    result = uploader.compose_vertical([first, second], label="strict-compose")

    assert result is None
    assert list(tmp_path.glob("strict-compose_*.jpg")) == []


def test_upload_read_timeout_resolves_unique_late_cloud_item_without_reupload(tmp_path: Path, monkeypatch):
    image = tmp_path / "capture.jpg"
    image.write_bytes(b"jpeg-data")
    config = SimpleNamespace(
        capture_dir=tmp_path,
        upload_url="http://cloud/api/wardrobe/add",
        upload_multi_url="http://cloud/api/wardrobe/add-multi",
        cloud_api_base="http://cloud",
        device_token="",
    )
    uploader = CameraUploader(config, GatewayState())
    calls = {"post": 0, "get": 0}

    def timeout_post(*args, **kwargs):
        calls["post"] += 1
        raise camera_module.requests.ReadTimeout("server kept processing after client timeout")

    class LateResponse:
        status_code = 200
        text = "ok"

        @staticmethod
        def json():
            return [
                {
                    "id": "item-late-8",
                    "status": "in-wardrobe",
                    "real_position": "C08",
                    "capture_session_id": "store-late-8",
                    "category": "jacket",
                }
            ]

    def late_get(*args, **kwargs):
        calls["get"] += 1
        return LateResponse()

    monkeypatch.setattr(camera_module.requests, "post", timeout_post)
    monkeypatch.setattr(camera_module.requests, "get", late_get)

    status, style_id, result = uploader.upload(
        image,
        {
            "capture_session_id": "store-late-8",
            "source": "board1-back",
            "real_position": "C08",
            "real_position_index": 7,
        },
    )

    assert status == camera_module.STATUS_OK
    assert style_id == 10
    assert result["ok"] is True
    assert result["late_resolved"] is True
    assert result["item_id"] == "item-late-8"
    assert calls == {"post": 1, "get": 1}
