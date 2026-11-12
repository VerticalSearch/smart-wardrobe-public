from __future__ import annotations

import threading
import time
from pathlib import Path
from typing import Any
from urllib.parse import quote

import requests
from fastapi import FastAPI, HTTPException, Request, Response
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse, HTMLResponse, JSONResponse, StreamingResponse
from fastapi.staticfiles import StaticFiles

from .camera import CameraUploader, style_id_from_response
from .care_schedule import CARE_DEVICE_DEFS, CareScheduler, CareScheduleStore
from .care_gravity import parse_care_gravity_reading
from .config import GatewayConfig
from .mqtt_bridge import MqttBridge
from .inventory import InventoryError, WardrobeInventory
from .protocol import (
    CMD_CAPTURE,
    CMD_IMAGE_READY,
    STATUS_CAM_FAIL,
    STATUS_UPLOAD_FAIL,
    action_from_payload,
    build_serial_commands,
    detection_packet,
    image_info_frame,
    result_frame,
    servo_line,
)
from .serial_link import SerialLink
from .state import gateway_state
from .weather_care import WeatherCareController, WeatherClient, is_humid_weather


config = GatewayConfig.from_env()
serial_link = SerialLink(config, gateway_state)
camera_uploader = CameraUploader(config, gateway_state)
mqtt_bridge: MqttBridge | None = None
last_uploaded_image: Path | None = None
care_schedule_store = CareScheduleStore(config.project_root / "data" / "care_schedules.json")
care_scheduler: CareScheduler | None = None
weather_client = WeatherClient(config.weather_latitude, config.weather_longitude, config.weather_location)
weather_care: WeatherCareController | None = None

CAMERA_SERVO_CHANNEL = 3
CAMERA_SERVO_HOME_ANGLE = 250
CAMERA_SERVO_SWEEP_ANGLES = [
    ("top", 235),
    ("middle", 250),
    ("bottom", 265),
]
CAMERA_SERVO_SETTLE_SECONDS = 0.65
STORE_POSITION_COUNT = 12
# Guard only the B-zone front/back capture burst, not the whole store workflow.
STORE_CAPTURE_SESSION_SECONDS = 45
STYLE_LABELS = {
    0: "样式待识别",
    1: "T恤",
    2: "衬衫",
    3: "外套",
    4: "连衣裙",
    5: "裤装",
    6: "半身裙",
    7: "短裤",
    8: "毛衣",
    9: "卫衣",
    10: "夹克",
    11: "鞋",
}
store_position_path = config.capture_dir.parent / "data" / "store_positions.json"
inventory = WardrobeInventory(store_position_path, slot_count=STORE_POSITION_COUNT)
store_capture_session_lock = threading.Lock()
store_capture_session: dict[str, Any] = {
    "id": "",
    "started_at": 0.0,
    "captures": 0,
    "front_pending": False,
    "front_capture": None,
    "paired_uploaded": False,
}
recognition_refresh_running = False
cloud_sync_running = False
cloud_sync_wakeup = threading.Event()
RECOGNITION_REFRESH_INTERVAL = 15.0
RECOGNITION_PLACEHOLDER_NAMES = {"新衣物", "", "未知", "待识别", "???"}

app = FastAPI(title="Smart Wardrobe Raspberry Pi Gateway", version="0.1.0")
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)

static_dir = Path(__file__).resolve().parents[2] / "static"
app.mount("/static", StaticFiles(directory=str(static_dir)), name="static")


def _task_type_from_action(action_id: int | None) -> str:
    if action_id in {6, 11, 12, 13}:
        return "store"
    if action_id == 7:
        return "fetch"
    if action_id == 14:
        return "fetch-reset"
    if action_id == 8:
        return "hot-air-start"
    if action_id == 9:
        return "hot-air-stop"
    if action_id == 10:
        return "brush"
    if action_id == 0:
        return "emergency"
    return "task"


def _parse_task_status_line(line: str) -> dict[str, Any] | None:
    if not line.startswith("@TASK|"):
        return None
    result: dict[str, str] = {}
    for part in line.split("|")[1:]:
        if "=" not in part:
            continue
        key, value = part.split("=", 1)
        result[key.strip()] = value.strip()
    action_id: int | None = None
    action_text = result.get("action") or result.get("action_id")
    if action_text:
        try:
            action_id = int(action_text)
        except ValueError:
            action_id = None
    task_type = result.get("type") or _task_type_from_action(action_id)
    return {
        "type": task_type,
        "action_id": action_id,
        "id": result.get("id", ""),
        "state": result.get("state", result.get("status", "")),
        "target_c": result.get("target_c", ""),
        "detail": result.get("detail", line),
    }


def _move_camera_servo(angle: int, command_id: str, label: str) -> bool:
    line = servo_line(CAMERA_SERVO_CHANNEL, angle, f"{command_id}-{label}").line
    ok = serial_link.write_line(line)
    gateway_state.add_log(
        "camera-servo",
        f"servo{CAMERA_SERVO_CHANNEL} {label} angle={angle}",
        {"ok": ok, "line": line},
    )
    return ok


def _store_position_label(index: int) -> str:
    return inventory.slot_label(index)


def _style_label(style_id: int) -> str:
    return STYLE_LABELS.get(style_id, "样式待识别")


def _capture_session_expired(now: float, started_at: Any) -> bool:
    try:
        return now - float(started_at or 0) > STORE_CAPTURE_SESSION_SECONDS
    except (TypeError, ValueError):
        return True


def _decide_board_capture_upload(side_id: int) -> tuple[bool, str]:
    if side_id != 0:
        return False, "board1-back-skipped-single-front"
    return True, "board1-front-single"


def _capture_frame_paths(upload: dict[str, Any] | None) -> list[Path]:
    if not isinstance(upload, dict):
        return []
    detail = upload.get("multi_angle")
    frames: list[Path] = []
    if isinstance(detail, dict):
        raw_frames = detail.get("frames")
        if isinstance(raw_frames, list):
            for item in raw_frames:
                path = Path(str(item))
                if path.exists() and path.stat().st_size > 0:
                    frames.append(path)
    if frames:
        return frames
    path_text = str(upload.get("path") or "")
    if path_text:
        path = Path(path_text)
        if path.exists() and path.stat().st_size > 0:
            frames.append(path)
    return frames


def _remember_board_front_capture(upload: dict[str, Any]) -> None:
    front_capture = {
        "path": upload.get("path"),
        "multi_angle": upload.get("multi_angle"),
        "captured_at": gateway_state._now(),
    }
    with store_capture_session_lock:
        store_capture_session["front_pending"] = False
        store_capture_session["front_capture"] = front_capture


def _clear_board_front_pending() -> None:
    with store_capture_session_lock:
        store_capture_session["front_pending"] = False


def _upload_board_capture_pair(back_upload: dict[str, Any], source: str) -> tuple[int, int, dict[str, Any]]:
    with store_capture_session_lock:
        if store_capture_session.get("paired_uploaded"):
            return 0, 0, back_upload
        front_capture = store_capture_session.get("front_capture")

    front_frames = _capture_frame_paths(front_capture if isinstance(front_capture, dict) else None)
    back_frames = _capture_frame_paths(back_upload)
    expected_side_frames = len(CAMERA_SERVO_SWEEP_ANGLES)
    if len(front_frames) != expected_side_frames or len(back_frames) != expected_side_frames:
        return STATUS_CAM_FAIL, 0, {
            "ok": False,
            "error": "front/back capture requires exactly three valid frames per side",
            "paired_multi_angle": {
                "source": source,
                "expected_front_frames": expected_side_frames,
                "expected_back_frames": expected_side_frames,
                "front_frames": [str(path) for path in front_frames],
                "back_frames": [str(path) for path in back_frames],
            },
        }
    pair_frames = front_frames + back_frames

    pair_path = camera_uploader.compose_vertical(pair_frames, label="multi_angle_board1-front-back")
    if pair_path is None:
        return STATUS_CAM_FAIL, 0, {
            "ok": False,
            "error": "front/back compose failed",
            "paired_multi_angle": {"source": source, "frames": [str(path) for path in pair_frames]},
        }

    cloud_metadata = inventory.active_store_context()
    if cloud_metadata is None:
        return STATUS_UPLOAD_FAIL, 0, {
            "ok": False,
            "error": "board store capture has no active inventory transaction",
            "paired_multi_angle": {"source": source, "composite": str(pair_path)},
        }
    cloud_metadata = {
        **cloud_metadata,
        "capture_session_id": cloud_metadata["command_id"],
        "source": source,
    }
    status, style_id, upload = camera_uploader.upload(pair_path, cloud_metadata=cloud_metadata)
    upload["paired_multi_angle"] = {
        "source": source,
        "front": front_capture if isinstance(front_capture, dict) else None,
        "back": back_upload.get("multi_angle"),
        "frames": [str(path) for path in pair_frames],
        "composite": str(pair_path),
    }
    _annotate_upload_position(upload, style_id)
    if status == 0 and upload.get("ok"):
        with store_capture_session_lock:
            store_capture_session["paired_uploaded"] = True
    return status, style_id, upload


def _assign_next_store_position(
    item_id: str,
    style_id: int = 0,
    style: str = "",
    command_id: str = "",
) -> dict[str, Any]:
    metadata: dict[str, Any] = {"assigned_at": gateway_state._now()}
    if style_id:
        metadata["style_id"] = style_id
    if style:
        metadata["style"] = style
    elif style_id:
        metadata["style"] = _style_label(style_id)
    if command_id:
        metadata["capture_session_id"] = command_id
        item_meta = inventory.bind_store_item(command_id, item_id, metadata)
    else:
        item_meta = inventory.bind_active_store_item(item_id, metadata)
    gateway_state.add_log(
        "store-position",
        f"{item_id} -> {item_meta['label']}",
        {"index": item_meta["index"]},
    )
    return item_meta


def _annotate_upload_position(upload: dict[str, Any], style_id: int = 0) -> dict[str, Any] | None:
    response = upload.get("response")
    if not isinstance(response, dict):
        return None
    item = response.get("item")
    if not isinstance(item, dict):
        return None
    item_id = str(item.get("id") or "")
    if not item_id:
        return None
    style = str(item.get("style") or item.get("category") or item.get("type") or "")
    cloud_metadata = upload.get("cloud_metadata")
    command_id = ""
    if isinstance(cloud_metadata, dict):
        command_id = str(cloud_metadata.get("capture_session_id") or "").strip()
    try:
        position = _assign_next_store_position(
            item_id,
            style_id=style_id,
            style=style,
            command_id=command_id,
        )
    except InventoryError as exc:
        gateway_state.add_log("store-position-error", str(exc), {"item_id": item_id})
        upload["inventory_error"] = str(exc)
        return None
    item["real_position"] = position["label"]
    item["real_position_index"] = position["index"]
    item["style_id"] = style_id
    item["style"] = style or _style_label(style_id)
    upload["real_position"] = position
    try:
        _post_cloud_item_status(item_id, "storing")
    except Exception as exc:
        gateway_state.add_log("inventory-cloud-storing-retry", str(exc), {"item_id": item_id})
    if style_id == 0 and item_id:
        threading.Thread(target=_quick_poll_recognition, args=(item_id,), daemon=True).start()
    return position


def _local_capture_upload_result(path: Path, source: str, detail_key: str, detail: dict[str, Any]) -> tuple[int, int, dict[str, Any]]:
    result = {
        "ok": True,
        "status_code": 0,
        "style_id": 0,
        "path": str(path),
        "local_only": True,
        detail_key: detail,
    }
    gateway_state.last_upload = result
    gateway_state.add_log("capture-local-only", source, {"path": str(path)})
    return 0, 0, result


def _capture_upload_with_camera_servo(
    command_id: str,
    source: str,
    *,
    upload_to_cloud: bool = False,
    cloud_metadata: dict[str, Any] | None = None,
) -> tuple[int, int, dict[str, Any]]:
    if upload_to_cloud and cloud_metadata is None:
        return STATUS_UPLOAD_FAIL, 0, {
            "ok": False,
            "error": "cloud capture upload requires a matching active inventory transaction",
        }
    frames: list[Path] = []
    servo_steps: list[dict[str, Any]] = []
    for label, angle in CAMERA_SERVO_SWEEP_ANGLES:
        ok = _move_camera_servo(angle, command_id, label)
        servo_steps.append({"label": label, "angle": angle, "ok": ok})
        time.sleep(CAMERA_SERVO_SETTLE_SECONDS)
        path = camera_uploader.capture()
        if path is not None:
            frames.append(path)

    _move_camera_servo(CAMERA_SERVO_HOME_ANGLE, command_id, "home")
    expected_frames = len(CAMERA_SERVO_SWEEP_ANGLES)
    if len(frames) != expected_frames:
        return STATUS_CAM_FAIL, 0, {
            "ok": False,
            "error": gateway_state.camera_error or f"multi-angle capture requires {expected_frames} valid frames",
            "multi_angle": {
                "source": source,
                "servo_channel": CAMERA_SERVO_CHANNEL,
                "steps": servo_steps,
                "expected_frames": expected_frames,
                "captured_frames": len(frames),
                "frames": [str(path) for path in frames],
            },
        }

    upload_path = camera_uploader.compose_vertical(frames, label=f"multi_angle_{source}")
    if upload_path is None:
        return STATUS_CAM_FAIL, 0, {
            "ok": False,
            "error": "multi-angle compose failed",
            "multi_angle": {"source": source, "servo_channel": CAMERA_SERVO_CHANNEL, "steps": servo_steps, "frames": [str(path) for path in frames]},
        }

    if not upload_to_cloud:
        return _local_capture_upload_result(
            upload_path,
            source,
            "multi_angle",
            {
                "source": source,
                "servo_channel": CAMERA_SERVO_CHANNEL,
                "home_angle": CAMERA_SERVO_HOME_ANGLE,
                "steps": servo_steps,
                "frames": [str(path) for path in frames],
                "composite": str(upload_path),
            },
        )

    status, style_id, upload = camera_uploader.upload(upload_path, cloud_metadata=cloud_metadata)
    upload["multi_angle"] = {
        "source": source,
        "servo_channel": CAMERA_SERVO_CHANNEL,
        "home_angle": CAMERA_SERVO_HOME_ANGLE,
        "steps": servo_steps,
        "frames": [str(path) for path in frames],
        "composite": str(upload_path),
    }
    _annotate_upload_position(upload, style_id)
    return status, style_id, upload


def _capture_upload_single(
    command_id: str,
    source: str,
    *,
    upload_to_cloud: bool = False,
    cloud_metadata: dict[str, Any] | None = None,
) -> tuple[int, int, dict[str, Any]]:
    if upload_to_cloud and cloud_metadata is None:
        return STATUS_UPLOAD_FAIL, 0, {
            "ok": False,
            "error": "cloud capture upload requires a matching active inventory transaction",
        }
    path = camera_uploader.capture()
    if path is None:
        return STATUS_CAM_FAIL, 0, {
            "ok": False,
            "error": gateway_state.camera_error or "single capture failed",
            "single_capture": {"source": source, "camera_servo_moved": False},
        }

    if not upload_to_cloud:
        return _local_capture_upload_result(
            path,
            source,
            "single_capture",
            {"source": source, "path": str(path), "camera_servo_moved": False},
        )

    status, style_id, upload = camera_uploader.upload(path, cloud_metadata=cloud_metadata)
    upload["single_capture"] = {
        "source": source,
        "path": str(path),
        "camera_servo_moved": False,
    }
    _annotate_upload_position(upload, style_id)
    return status, style_id, upload


def _capture_board_store_image(side_id: int) -> tuple[int, int, dict[str, Any], bool, str]:
    """Capture exactly one front image for a store transaction.

    Board2 still sends a back-side trigger as part of its legacy motion path;
    single-front mode answers that trigger explicitly without capturing or
    composing another image.
    """
    upload_to_cloud, source = _decide_board_capture_upload(side_id)
    if not upload_to_cloud:
        return 0, 0, {
            "ok": True,
            "status_code": 0,
            "style_id": 0,
            "local_only": True,
            "capture_mode": "single-front",
            "skipped": True,
            "reason": "back capture disabled in single-front mode",
        }, False, source

    active = inventory.active_store_context()
    if active is None:
        return STATUS_UPLOAD_FAIL, 0, {
            "ok": False,
            "error": "single-front store capture has no active inventory transaction",
            "capture_mode": "single-front",
        }, True, source
    if active.get("item_id"):
        return STATUS_UPLOAD_FAIL, 0, {
            "ok": False,
            "error": f"active store transaction is already bound to item {active['item_id']}",
            "capture_mode": "single-front",
        }, True, source

    metadata = {
        **active,
        "capture_session_id": active["command_id"],
        "source": source,
    }
    status, style_id, upload = _capture_upload_single(
        "board1-capture-front",
        source,
        upload_to_cloud=True,
        cloud_metadata=metadata,
    )
    upload["capture_mode"] = "single-front"
    return status, style_id, upload, True, source


def _use_multi_angle_capture(payload: dict[str, Any]) -> bool:
    mode = str(payload.get("mode", "")).strip().lower()
    if mode in {"single", "fixed", "center"}:
        return False
    if payload.get("multi_angle") is False or payload.get("sweep") is False:
        return False
    return True


def _should_upload_capture(payload: dict[str, Any]) -> bool:
    return payload.get("upload") is True or payload.get("upload_to_cloud") is True


def _capture_cloud_metadata(payload: dict[str, Any], source: str) -> dict[str, Any] | None:
    if not _should_upload_capture(payload):
        return None
    active = inventory.active_store_context()
    if active is None:
        raise InventoryError("cloud capture upload requires an active store inventory transaction")
    requested_command_id = str(
        payload.get("inventory_command_id") or payload.get("store_command_id") or ""
    ).strip()
    if not requested_command_id:
        raise InventoryError("cloud capture upload requires inventory_command_id")
    if requested_command_id != active["command_id"]:
        raise InventoryError(
            f"capture inventory command {requested_command_id} does not match active store transaction {active['command_id']}"
        )
    if active.get("item_id"):
        raise InventoryError(f"active store transaction is already bound to item {active['item_id']}")
    return {
        **active,
        "capture_session_id": active["command_id"],
        "source": source,
    }


def _annotate_wardrobe_items(items: Any) -> Any:
    if not isinstance(items, list):
        return items
    reconciliation = inventory.reconcile_cloud_items(items)
    if reconciliation["imported"]:
        gateway_state.add_log("inventory-cloud-import", str(reconciliation["imported"]))
    if reconciliation["recovered"]:
        gateway_state.add_log("inventory-cloud-late-bind", str(reconciliation["recovered"]))
        cloud_sync_wakeup.set()
    if reconciliation["conflicts"]:
        gateway_state.add_log("inventory-cloud-conflict", str(reconciliation["conflicts"]))
    inventory.annotate_items(items)
    conflicts_by_item = {
        str(conflict.get("item_id") or ""): conflict
        for conflict in reconciliation["conflicts"]
        if str(conflict.get("item_id") or "") and "," not in str(conflict.get("item_id") or "")
    }
    for item in items:
        if not isinstance(item, dict):
            continue
        item_id = str(item.get("id") or "")
        conflict = conflicts_by_item.get(item_id)
        if conflict is not None:
            item["inventory_conflict"] = conflict
            item["status"] = "inventory-error"
        cloud_name = str(item.get("name") or "")
        cloud_image_status = str(item.get("image_status") or "")
        if cloud_image_status == "done" and cloud_name and cloud_name not in RECOGNITION_PLACEHOLDER_NAMES:
            cloud_sid = style_id_from_response(item)
            if cloud_sid and cloud_sid != 0:
                if not item.get("style_id") or item.get("style_id") == 0:
                    item["style_id"] = cloud_sid
                if not item.get("style"):
                    item["style"] = cloud_name
                inventory.update_item_metadata(item_id, style_id=cloud_sid, style=cloud_name)
        style_id = int(item.get("style_id") or item.get("styleId") or 0)
        if not item.get("style") and not item.get("category"):
            item["style"] = _style_label(style_id)
    return items


def _refresh_recognition_styles() -> None:
    global recognition_refresh_running
    while recognition_refresh_running:
        time.sleep(RECOGNITION_REFRESH_INTERVAL)
        if not recognition_refresh_running:
            break
        try:
            r = requests.get(f"{config.cloud_api_base}/api/wardrobe", timeout=15)
            if r.status_code != 200:
                continue
            items = r.json()
            if not isinstance(items, list):
                continue
            reconciliation = inventory.reconcile_cloud_items(items)
            if reconciliation["recovered"]:
                gateway_state.add_log("inventory-cloud-late-bind", str(reconciliation["recovered"]))
                cloud_sync_wakeup.set()
            for item in items:
                if not isinstance(item, dict):
                    continue
                item_id = str(item.get("id") or "")
                cloud_status = str(item.get("image_status") or "")
                cloud_name = str(item.get("name") or "")
                if cloud_status != "done" or cloud_name in RECOGNITION_PLACEHOLDER_NAMES:
                    continue
                cloud_sid = style_id_from_response(item)
                if not cloud_sid:
                    continue
                updated = inventory.update_item_metadata(item_id, style_id=cloud_sid, style=cloud_name)
                if updated is not None:
                    gateway_state.add_log("recognition-refresh", f"{item_id} -> style={cloud_sid} name={cloud_name}")
        except Exception:
            pass


def _start_recognition_refresh() -> None:
    global recognition_refresh_running
    if recognition_refresh_running:
        return
    recognition_refresh_running = True
    t = threading.Thread(target=_refresh_recognition_styles, daemon=True)
    t.start()


def _stop_recognition_refresh() -> None:
    global recognition_refresh_running
    recognition_refresh_running = False


def _quick_poll_recognition(item_id: str, timeout: float = 45.0) -> None:
    started = time.time()
    while time.time() - started < timeout:
        time.sleep(3.0)
        try:
            r = requests.get(f"{config.cloud_api_base}/api/wardrobe", timeout=10)
            if r.status_code != 200:
                continue
            items = r.json()
            if not isinstance(items, list):
                continue
            for item in items:
                if not isinstance(item, dict):
                    continue
                if str(item.get("id") or "") != item_id:
                    continue
                cloud_status = str(item.get("image_status") or "")
                cloud_name = str(item.get("name") or "")
                if cloud_status != "done" or cloud_name in RECOGNITION_PLACEHOLDER_NAMES:
                    continue
                cloud_sid = style_id_from_response(item)
                if not cloud_sid:
                    continue
                if inventory.update_item_metadata(item_id, style_id=cloud_sid, style=cloud_name) is not None:
                    gateway_state.add_log("recognition-poll", f"{item_id} -> style={cloud_sid} name={cloud_name}")
                    return
        except Exception:
            pass
    gateway_state.add_log("recognition-poll-timeout", item_id)


def _reset_store_capture_session(command_id: str = "") -> None:
    with store_capture_session_lock:
        store_capture_session.update(
            {
                "id": command_id,
                "started_at": time.time() if command_id else 0.0,
                "captures": 0,
                "front_pending": False,
                "front_capture": None,
                "paired_uploaded": False,
                "reason": "inventory-store-transaction" if command_id else "",
            }
        )


def _prepare_inventory_transaction(payload: dict[str, Any], command_id: str) -> dict[str, Any] | None:
    if action_from_payload(payload) != "todo":
        return None
    try:
        action_id = int(payload.get("action_id", payload.get("value", 0)) or 0)
    except (TypeError, ValueError) as exc:
        raise InventoryError("invalid action_id") from exc
    target_c = payload.get("target_c", payload.get("real_position"))

    if action_id in {6, 11}:
        transaction = inventory.reserve_store(command_id, action_id, target_c)
        payload["target_c"] = transaction["slot"]
        _reset_store_capture_session(command_id)
    elif action_id == 7:
        item_id = str(payload.get("item_id") or "").strip()
        item_ids = payload.get("item_ids")
        if not item_id and isinstance(item_ids, list):
            clean_ids = [str(value).strip() for value in item_ids if str(value).strip()]
            if len(clean_ids) != 1:
                raise InventoryError("action 7 accepts exactly one garment per transaction")
            item_id = clean_ids[0]
        transaction = inventory.reserve_fetch(command_id, action_id, item_id=item_id, target_c=target_c)
        payload["item_id"] = transaction["item_id"]
        payload["target_c"] = transaction["slot"]
    else:
        return None

    gateway_state.add_log(
        "inventory-reserve",
        f"{transaction['kind']} {transaction['slot']}",
        {"id": command_id, "item_id": transaction.get("item_id", ""), "action_id": action_id},
    )
    return transaction


def _direct_inventory_action_id(payload: dict[str, Any]) -> int | None:
    if action_from_payload(payload) != "todo":
        return None
    try:
        action_id = int(payload.get("action_id", payload.get("value", 0)) or 0)
    except (TypeError, ValueError):
        return None
    return action_id if action_id in {6, 7, 11} else None


def _send_commands(payload: dict[str, Any], source: str = "http") -> dict[str, Any]:
    action = action_from_payload(payload)
    command_id = str(payload.get("id", f"{source}-{int(time.time() * 1000)}"))
    payload.setdefault("id", command_id)
    gateway_state.last_command = {"source": source, "payload": payload, "time": gateway_state._now()}

    if action == "status":
        return {"ok": True, "detail": "alive", "commands": []}

    if action == "capture":
        upload_to_cloud = _should_upload_capture(payload)
        cloud_metadata = _capture_cloud_metadata(payload, "http-capture")
        if _use_multi_angle_capture(payload):
            status, style_id, upload = _capture_upload_with_camera_servo(
                command_id,
                "http-capture",
                upload_to_cloud=upload_to_cloud,
                cloud_metadata=cloud_metadata,
            )
        else:
            status, style_id, upload = _capture_upload_single(
                command_id,
                "http-capture",
                upload_to_cloud=upload_to_cloud,
                cloud_metadata=cloud_metadata,
            )
        global last_uploaded_image
        if upload.get("path"):
            last_uploaded_image = Path(str(upload["path"]))
        serial_link.write_bytes(result_frame(status, style_id), display=f"RESULT status={status} style={style_id}")
        gateway_state.add_task_event(
            task_type="vision-store",
            action_id=None,
            command_id=command_id,
            state="done" if status == 0 else "error",
            source=source,
            detail=f"capture status={status}, style={style_id}",
        )
        return {"ok": status == 0, "detail": f"capture status={status}, style={style_id}", "status": status, "style_id": style_id, "upload": upload}

    inventory_transaction = _prepare_inventory_transaction(payload, command_id)
    if inventory_transaction is not None and inventory_transaction.get("state") == "running":
        return {
            "ok": True,
            "detail": "inventory transaction already dispatched",
            "commands": [],
            "failed": [],
            "inventory": inventory_transaction,
        }
    commands = build_serial_commands(payload)
    sent: list[str] = []
    failed: list[str] = []
    for command in commands:
        if serial_link.write_line(command.line):
            sent.append(command.line)
            time.sleep(0.02)
        else:
            failed.append(command.line)

    ok = not failed
    if inventory_transaction is not None:
        if ok:
            inventory.mark_dispatched(command_id)
        else:
            inventory.cancel_dispatch(command_id, f"serial send failed: {', '.join(failed)}")
            if inventory_transaction.get("kind") == "store":
                _reset_store_capture_session()
    if action == "todo":
        try:
            action_id = int(payload.get("action_id", payload.get("value", 0)) or 0)
        except (TypeError, ValueError):
            action_id = None
        gateway_state.add_task_event(
            task_type=_task_type_from_action(action_id),
            action_id=action_id,
            command_id=command_id,
            state="sent" if ok else "send_failed",
            source=source,
            detail=f"sent={len(sent)}, failed={len(failed)}",
        )
    if action == "board1_output":
        gateway_state.add_task_event(
            task_type=f"output-{payload.get('device', '')}",
            action_id=None,
            command_id=command_id,
            state="sent" if ok else "send_failed",
            source=source,
            detail=f"sent={len(sent)}, failed={len(failed)}",
        )
    return {
        "ok": ok,
        "detail": f"sent={len(sent)}, failed={len(failed)}",
        "commands": sent,
        "failed": failed,
        "inventory": inventory_transaction,
    }


def _handle_serial_event(event: dict[str, Any]) -> None:
    event_type = event.get("type")
    if event_type == "line":
        line = str(event.get("line", ""))
        gravity = parse_care_gravity_reading(line)
        if gravity is not None:
            gateway_state.add_care_gravity_sample(left=gravity.left, right=gravity.right, source="serial", unit=gravity.unit)
            gateway_state.add_log(
                "care-gravity",
                f"left={gravity.left if gravity.left is not None else '--'}, right={gravity.right if gravity.right is not None else '--'}, unit={gravity.unit}",
            )
            if mqtt_bridge is not None:
                mqtt_bridge.publish_status("care_gravity", line, stage="board1_gravity")
            return
        task_status = _parse_task_status_line(line)
        if task_status is not None:
            transaction = None
            try:
                transaction = inventory.apply_task_status(
                    str(task_status.get("id", "")),
                    task_status.get("action_id"),
                    str(task_status.get("state", "")),
                    task_status.get("target_c"),
                )
            except InventoryError as exc:
                gateway_state.add_log("inventory-status-error", str(exc), {"line": line})
            if isinstance(transaction, dict):
                transaction_state = str(transaction.get("state") or "")
                cloud_sync = transaction.get("cloud_sync")
                if isinstance(cloud_sync, dict) and cloud_sync.get("state") in {"pending", "retry"}:
                    cloud_sync_wakeup.set()
                if transaction.get("kind") == "store" and transaction_state in {"done", "error", "stopped"}:
                    _reset_store_capture_session()
            gateway_state.add_task_event(
                task_type=str(task_status.get("type", "")),
                action_id=task_status.get("action_id"),
                command_id=str(task_status.get("id", "")),
                state=str(task_status.get("state", "")),
                source="board1",
                detail=str(task_status.get("detail", line)),
            )
            if mqtt_bridge is not None:
                mqtt_bridge.publish_status("task", line, stage="board1_task")
            return
        if mqtt_bridge is not None and line.startswith("@DBG1|"):
            mqtt_bridge.publish_status("board1", line, stage="board1_dbg")
        elif mqtt_bridge is not None and line.startswith("@DBG2|"):
            mqtt_bridge.publish_status("board2", line, stage="board2_dbg")
        return

    if event_type == "frame" and event.get("cmd") == CMD_CAPTURE:
        side_id = int(event.get("p1") or 0)
        status, style_id, upload, upload_to_cloud, capture_source = _capture_board_store_image(side_id)
        gateway_state.add_log(
            "capture-request",
            f"board1 requested capture side={side_id} upload={int(upload_to_cloud)}",
            {"source": capture_source, "capture_mode": "single-front"},
        )
        global last_uploaded_image
        if upload.get("path"):
            last_uploaded_image = Path(str(upload["path"]))
        serial_link.write_bytes(result_frame(status, style_id), display=f"RESULT status={status} style={style_id}")
        gateway_state.add_task_event(
            task_type="vision-store",
            action_id=None,
            command_id="board1-capture",
            state="done" if status == 0 else "error",
            source="board1",
            detail=f"capture side={side_id}, upload={int(upload_to_cloud)}, status={status}, style={style_id}",
        )
        if mqtt_bridge is not None:
            mqtt_bridge.publish_status("capture_done", f"status={status},style={style_id}", stage="uart_rx")
        return

    if event_type == "frame" and event.get("cmd") == CMD_IMAGE_READY:
        ready = int(event.get("p1") or 0)
        if ready == 1 and last_uploaded_image and last_uploaded_image.exists():
            data = last_uploaded_image.read_bytes()
            serial_link.write_bytes(data, display=f"IMAGE_BYTES size={len(data)}")
            gateway_state.add_log("image-send", f"sent {len(data)} bytes")
        else:
            gateway_state.add_log("image-send-skip", "board1 not ready or no image")
        return


def _cloud_headers() -> dict[str, str]:
    headers = {"Content-Type": "application/json"}
    if config.device_token:
        headers["Authorization"] = f"Bearer {config.device_token}"
    return headers


def _cloud_response_body(response: requests.Response) -> Any:
    try:
        return response.json()
    except Exception:
        return response.text[:300]


def _cloud_task_id(body: Any) -> str:
    if not isinstance(body, dict):
        return ""
    direct = str(body.get("id") or body.get("task_id") or "").strip()
    if direct:
        return direct
    task = body.get("task")
    if isinstance(task, dict):
        return str(task.get("id") or task.get("task_id") or "").strip()
    return ""


def _find_cloud_task_for_command(command_id: str) -> tuple[str, dict[str, Any] | None]:
    try:
        response = requests.get(
            f"{config.cloud_api_base}/api/tasks",
            headers=_cloud_headers(),
            timeout=10,
        )
        if response.status_code < 200 or response.status_code >= 300:
            return "", None
        body = response.json()
        if not isinstance(body, list):
            return "", None
        matches = []
        for task in body:
            if not isinstance(task, dict):
                continue
            payload = task.get("payload")
            if isinstance(payload, dict) and str(payload.get("gateway_command_id") or "") == command_id:
                matches.append(task)
        if len(matches) != 1:
            return "", None
        return str(matches[0].get("id") or ""), matches[0]
    except Exception:
        return "", None


def _rollback_cloud_task(task_id: str, task_payload: dict[str, Any], error: str) -> dict[str, Any]:
    task_id = str(task_id or "").strip()
    if not task_id:
        return {"ok": False, "state": "unresolved", "error": "cloud task id is unavailable"}
    target = f"{config.cloud_api_base}/api/tasks/{quote(task_id, safe='')}"
    try:
        response = requests.delete(target, headers=_cloud_headers(), timeout=15)
        if 200 <= response.status_code < 300:
            gateway_state.add_log("cloud-task-rollback", f"deleted {task_id}", {"error": error})
            return {"ok": True, "state": "deleted", "task_id": task_id}
        delete_error = f"HTTP {response.status_code}: {response.text[:160]}"
    except Exception as exc:
        delete_error = str(exc)

    fallback = {
        "type": str(task_payload.get("type") or ""),
        "status": "send_failed",
        "item_ids": list(task_payload.get("item_ids") or []),
        "payload": {**dict(task_payload.get("payload") or {}), "serial_error": error},
    }
    try:
        response = requests.post(target, json=fallback, headers=_cloud_headers(), timeout=15)
        if 200 <= response.status_code < 300:
            gateway_state.add_log(
                "cloud-task-rollback",
                f"marked send_failed {task_id}",
                {"delete_error": delete_error, "error": error},
            )
            return {
                "ok": True,
                "state": "send_failed",
                "task_id": task_id,
                "delete_error": delete_error,
            }
        fallback_error = f"HTTP {response.status_code}: {response.text[:160]}"
    except Exception as exc:
        fallback_error = str(exc)
    gateway_state.add_log(
        "cloud-task-rollback-error",
        f"{task_id}: {fallback_error}",
        {"delete_error": delete_error, "serial_error": error},
    )
    return {
        "ok": False,
        "state": "rollback-failed",
        "task_id": task_id,
        "delete_error": delete_error,
        "error": fallback_error,
    }


def _dispatch_hardware_task(task: dict[str, Any], command: dict[str, Any]) -> dict[str, Any]:
    command_id = str(command.get("id") or "").strip()
    if not command_id:
        raise InventoryError("hardware task dispatch requires command.id")
    item_ids = task.get("item_ids")
    if not isinstance(item_ids, list):
        raise InventoryError("hardware task dispatch requires task.item_ids as a list")
    task_payload = {
        "type": str(task.get("type") or "").strip(),
        "item_ids": item_ids,
        "payload": {**dict(task.get("payload") or {}), "gateway_command_id": command_id},
    }
    if not task_payload["type"]:
        raise InventoryError("hardware task dispatch requires task.type")

    task_id = ""
    created_body: Any = None
    create_error = ""
    try:
        response = requests.post(
            f"{config.cloud_api_base}/api/tasks",
            json=task_payload,
            headers=_cloud_headers(),
            timeout=15,
        )
        created_body = _cloud_response_body(response)
        if response.status_code < 200 or response.status_code >= 300:
            create_error = f"HTTP {response.status_code}: {response.text[:160]}"
        else:
            task_id = _cloud_task_id(created_body)
    except requests.Timeout as exc:
        create_error = f"cloud task create timed out: {exc}"
        task_id, created_task = _find_cloud_task_for_command(command_id)
        if created_task is not None:
            created_body = created_task
    except Exception as exc:
        create_error = str(exc)

    if create_error:
        rollback = _rollback_cloud_task(task_id, task_payload, create_error) if task_id else None
        return {
            "ok": False,
            "taskOk": False,
            "taskCreated": bool(task_id),
            "serialOk": False,
            "commandId": command_id,
            "taskId": task_id,
            "detail": create_error,
            "rollback": rollback,
        }
    if not task_id:
        task_id, created_task = _find_cloud_task_for_command(command_id)
        if created_task is not None:
            created_body = created_task
    if not task_id:
        return {
            "ok": False,
            "taskOk": False,
            "taskCreated": True,
            "serialOk": False,
            "commandId": command_id,
            "taskId": "",
            "detail": "cloud task was created but its id could not be resolved; serial dispatch was blocked",
            "rollback": {"ok": False, "state": "unresolved"},
        }

    try:
        serial_result = _send_commands(dict(command), source="hardware-task")
    except Exception as exc:
        inventory.cancel_dispatch(command_id, str(exc))
        serial_result = {"ok": False, "detail": str(exc), "commands": [], "failed": []}
    serial_ok = bool(serial_result.get("ok"))
    rollback = None
    if not serial_ok:
        rollback = _rollback_cloud_task(task_id, task_payload, str(serial_result.get("detail") or "serial dispatch failed"))
    gateway_state.add_log(
        "hardware-task-dispatch",
        f"task={task_id} command={command_id} serial_ok={int(serial_ok)}",
        {"rollback": rollback},
    )
    return {
        "ok": serial_ok,
        "taskOk": serial_ok,
        "taskCreated": True,
        "serialOk": serial_ok,
        "commandId": command_id,
        "taskId": task_id,
        "detail": str(serial_result.get("detail") or ""),
        "task": created_body,
        "serial": serial_result,
        "rollback": rollback,
    }


def _post_cloud_item_status(item_id: str, status_value: str) -> None:
    target = f"{config.cloud_api_base}/api/wardrobe/{quote(item_id, safe='')}/status"
    response = requests.post(
        target,
        json={"status": status_value},
        headers=_cloud_headers(),
        timeout=15,
    )
    if response.status_code < 200 or response.status_code >= 300:
        raise RuntimeError(f"HTTP {response.status_code}: {response.text[:160]}")


def _clear_cloud_wardrobe_items(item_ids: list[Any]) -> dict[str, Any]:
    normalized = inventory.validate_cloud_delete(item_ids)
    deleted: list[str] = []
    released_slots: list[dict[str, str]] = []
    cancelled_cloud_updates: list[dict[str, str]] = []
    failures: list[dict[str, str]] = []
    for item_id in normalized:
        try:
            _post_cloud_item_status(item_id, "deleted")
        except Exception as exc:
            failures.append({"item_id": item_id, "phase": "cloud-status", "error": str(exc)})
            continue
        try:
            local_result = inventory.mark_cloud_items_deleted([item_id], reason="cloud-delete")
        except InventoryError as exc:
            failures.append({"item_id": item_id, "phase": "local-release", "error": str(exc)})
            gateway_state.add_log(
                "inventory-cloud-delete-error",
                str(exc),
                {"item_id": item_id, "cloud_deleted": True},
            )
            continue
        deleted.append(item_id)
        released_slots.extend(local_result["released_slots"])
        cancelled_cloud_updates.extend(local_result["cancelled_cloud_updates"])
        gateway_state.add_log(
            "inventory-cloud-delete",
            f"{item_id} deleted; released {len(local_result['released_slots'])} slot(s)",
            {"released_slots": local_result["released_slots"]},
        )
    return {
        "ok": not failures,
        "requested": normalized,
        "deleted": deleted,
        "released_slots": released_slots,
        "cancelled_cloud_updates": cancelled_cloud_updates,
        "failures": failures,
    }


def _cloud_item_has_position(item_id: str, real_position: str) -> bool:
    response = requests.get(
        f"{config.cloud_api_base}/api/wardrobe",
        headers=_cloud_headers(),
        timeout=15,
    )
    if response.status_code < 200 or response.status_code >= 300:
        raise RuntimeError(f"HTTP {response.status_code}: {response.text[:160]}")
    items = response.json()
    if not isinstance(items, list):
        return False
    expected = inventory.normalize_slot(real_position)
    for item in items:
        if not isinstance(item, dict) or str(item.get("id") or "") != item_id:
            continue
        try:
            return inventory.normalize_slot(item.get("real_position")) == expected
        except InventoryError:
            return False
    return False


def _sync_pending_cloud_statuses() -> None:
    for update in inventory.pending_cloud_updates():
        command_id = str(update.get("command_id") or "")
        item_id = str(update.get("item_id") or "")
        status_value = str(update.get("status") or "")
        real_position = str(update.get("real_position") or "")
        if not command_id or not item_id or not status_value:
            inventory.mark_cloud_sync(command_id, False, "incomplete cloud sync payload")
            continue
        try:
            _post_cloud_item_status(item_id, status_value)
            if real_position and not _cloud_item_has_position(item_id, real_position):
                raise RuntimeError(f"cloud item {item_id} did not retain {real_position}")
            inventory.mark_cloud_sync(command_id, True)
            gateway_state.add_log(
                "inventory-cloud-sync",
                f"{item_id} -> {status_value} @ {real_position or '--'}",
                {"id": command_id},
            )
        except Exception as exc:
            inventory.mark_cloud_sync(command_id, False, str(exc))
            gateway_state.add_log("inventory-cloud-retry", str(exc), {"id": command_id, "item_id": item_id})


def _cloud_sync_worker() -> None:
    global cloud_sync_running
    while cloud_sync_running:
        _sync_pending_cloud_statuses()
        cloud_sync_wakeup.wait(10.0)
        cloud_sync_wakeup.clear()


def _start_cloud_sync() -> None:
    global cloud_sync_running
    if cloud_sync_running:
        return
    cloud_sync_running = True
    threading.Thread(target=_cloud_sync_worker, daemon=True).start()


def _reconcile_cloud_inventory_once() -> None:
    try:
        response = requests.get(
            f"{config.cloud_api_base}/api/wardrobe",
            headers=_cloud_headers(),
            timeout=15,
        )
        if response.status_code < 200 or response.status_code >= 300:
            raise RuntimeError(f"HTTP {response.status_code}: {response.text[:160]}")
        items = response.json()
        if not isinstance(items, list):
            raise RuntimeError("cloud wardrobe response is not a list")
        result = inventory.reconcile_cloud_items(items)
        if result["recovered"]:
            cloud_sync_wakeup.set()
        gateway_state.add_log("inventory-cloud-reconcile", str(result))
    except Exception as exc:
        gateway_state.add_log("inventory-cloud-reconcile-error", str(exc))


def _stop_cloud_sync() -> None:
    global cloud_sync_running
    cloud_sync_running = False
    cloud_sync_wakeup.set()


@app.on_event("startup")
def on_startup() -> None:
    serial_link.set_rx_handler(_handle_serial_event)
    serial_link.open()
    global mqtt_bridge
    mqtt_bridge = MqttBridge(config, gateway_state, _send_commands)
    mqtt_bridge.start()
    global care_scheduler
    care_scheduler = CareScheduler(care_schedule_store, _send_commands)
    care_scheduler.start()
    global weather_care
    weather_care = WeatherCareController(
        weather_client,
        _send_commands,
        enabled=config.weather_enable,
        humidity_threshold=config.weather_humidity_threshold,
        precipitation_probability_threshold=config.weather_precipitation_probability_threshold,
        duration_minutes=config.weather_care_duration_minutes,
        interval_seconds=config.weather_care_interval_seconds,
        cooldown_seconds=config.weather_care_cooldown_seconds,
    )
    weather_care.start()
    _start_recognition_refresh()
    _start_cloud_sync()
    threading.Thread(target=_reconcile_cloud_inventory_once, daemon=True).start()


@app.on_event("shutdown")
def on_shutdown() -> None:
    _stop_cloud_sync()
    if care_scheduler is not None:
        care_scheduler.stop()
    if weather_care is not None:
        weather_care.stop()
    if mqtt_bridge is not None:
        mqtt_bridge.stop()
    _stop_recognition_refresh()
    serial_link.close()


@app.get("/", response_class=HTMLResponse)
def display_page() -> Response:
    return FileResponse(str(static_dir / "display.html"), media_type="text/html")


@app.get("/cloud", response_class=HTMLResponse)
@app.get("/cloud-display", response_class=HTMLResponse)
def cloud_display_page() -> Response:
    packaged_cloud_display = static_dir / "cloud_display.html"
    headers = {
        "Cache-Control": "no-store, max-age=0",
        "Pragma": "no-cache",
    }
    if config.display_html.exists():
        return FileResponse(str(config.display_html), media_type="text/html", headers=headers)
    if packaged_cloud_display.exists():
        return FileResponse(str(packaged_cloud_display), media_type="text/html", headers=headers)
    return FileResponse(str(static_dir / "display.html"), media_type="text/html", headers=headers)


@app.get("/debug", response_class=HTMLResponse)
def debug_page() -> Response:
    return FileResponse(str(static_dir / "debug.html"), media_type="text/html")


@app.post("/api/control")
async def control(payload: dict[str, Any]) -> JSONResponse:
    action_id = _direct_inventory_action_id(payload)
    if action_id is not None:
        detail = (
            f"inventory action {action_id} cannot use /api/control; "
            "use /api/hardware/task-dispatch"
        )
        gateway_state.add_log("control-rejected", detail, {"payload": payload})
        raise HTTPException(status_code=409, detail=detail)
    try:
        result = _send_commands(payload, source="http")
        if mqtt_bridge is not None:
            mqtt_bridge.publish_status("sent" if result["ok"] else "error", result["detail"], str(payload.get("id", "")))
        return JSONResponse(result)
    except Exception as exc:
        gateway_state.add_log("control-error", str(exc), {"payload": payload})
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.post("/api/tasks")
async def reject_legacy_task_create(payload: dict[str, Any]) -> JSONResponse:
    detail = "direct task creation is disabled; use /api/hardware/task-dispatch"
    gateway_state.add_log("legacy-task-rejected", detail, {"payload": payload})
    raise HTTPException(status_code=409, detail=detail)


@app.post("/api/hardware/task-dispatch")
async def hardware_task_dispatch(payload: dict[str, Any]) -> JSONResponse:
    task = payload.get("task")
    command = payload.get("command")
    if not isinstance(task, dict) or not isinstance(command, dict):
        raise HTTPException(status_code=422, detail="task and command objects are required")
    try:
        return JSONResponse(_dispatch_hardware_task(task, command))
    except InventoryError as exc:
        raise HTTPException(status_code=422, detail=str(exc)) from exc


@app.post("/api/camera/capture-upload")
async def capture_upload(payload: dict[str, Any] | None = None) -> JSONResponse:
    payload = payload or {}
    command_id = str(payload.get("id") or "manual-capture")
    source = str(payload.get("source") or "http")
    upload_to_cloud = _should_upload_capture(payload)
    try:
        cloud_metadata = _capture_cloud_metadata(payload, source)
    except InventoryError as exc:
        raise HTTPException(status_code=409, detail=str(exc)) from exc
    if _use_multi_angle_capture(payload):
        status, style_id, upload = _capture_upload_with_camera_servo(
            command_id,
            source,
            upload_to_cloud=upload_to_cloud,
            cloud_metadata=cloud_metadata,
        )
    else:
        status, style_id, upload = _capture_upload_single(
            command_id,
            source,
            upload_to_cloud=upload_to_cloud,
            cloud_metadata=cloud_metadata,
        )
    global last_uploaded_image
    if upload.get("path"):
        last_uploaded_image = Path(str(upload["path"]))
    serial_link.write_bytes(result_frame(status, style_id), display=f"RESULT status={status} style={style_id}")
    gateway_state.add_task_event(
        task_type="vision-store",
        action_id=None,
        command_id=command_id,
        state="done" if status == 0 else "error",
        source=source,
        detail=f"capture status={status}, style={style_id}",
    )
    return JSONResponse({"ok": status == 0, "status": status, "style_id": style_id, "upload": upload})


@app.get("/api/wardrobe")
def wardrobe_list() -> Response:
    target = f"{config.cloud_api_base}/api/wardrobe"
    try:
        upstream = requests.get(target, timeout=30)
        try:
            body: Any = upstream.json()
        except Exception:
            body = upstream.text
    except Exception as exc:
        return JSONResponse({"ok": False, "error": str(exc), "target": target}, status_code=502)
    if isinstance(body, list):
        return JSONResponse(_annotate_wardrobe_items(body), status_code=upstream.status_code)
    return Response(
        content=upstream.content,
        status_code=upstream.status_code,
        media_type=upstream.headers.get("content-type"),
    )


@app.post("/api/wardrobe/clear")
async def clear_wardrobe(payload: dict[str, Any]) -> JSONResponse:
    item_ids = payload.get("item_ids")
    if not isinstance(item_ids, list):
        raise HTTPException(status_code=422, detail="item_ids must be a list")
    try:
        result = _clear_cloud_wardrobe_items(item_ids)
    except InventoryError as exc:
        raise HTTPException(status_code=409, detail=str(exc)) from exc
    return JSONResponse(result, status_code=200 if result["ok"] else 207)


@app.post("/api/wardrobe/{item_id}/status")
async def wardrobe_item_status(item_id: str, payload: dict[str, Any]) -> JSONResponse:
    status_value = str(payload.get("status") or "").strip().lower()
    if not status_value:
        raise HTTPException(status_code=422, detail="status is required")
    if status_value == "deleted":
        try:
            result = _clear_cloud_wardrobe_items([item_id])
        except InventoryError as exc:
            raise HTTPException(status_code=409, detail=str(exc)) from exc
        return JSONResponse(result, status_code=200 if result["ok"] else 502)
    try:
        _post_cloud_item_status(item_id, status_value)
    except Exception as exc:
        raise HTTPException(status_code=502, detail=str(exc)) from exc
    return JSONResponse({"ok": True, "item_id": item_id, "status": status_value})


@app.get("/api/inventory")
def inventory_status() -> JSONResponse:
    return JSONResponse(inventory.snapshot())


@app.post("/api/inventory/recover")
async def inventory_recover(payload: dict[str, Any]) -> JSONResponse:
    try:
        command_id = str(payload.get("command_id") or payload.get("id") or "")
        if command_id:
            recovered: dict[str, Any] = inventory.resolve_error(
                command_id,
                str(payload.get("outcome") or ""),
            )
        else:
            recovered = inventory.resolve_slot_error(
                payload.get("target_c"),
                str(payload.get("outcome") or ""),
                str(payload.get("item_id") or ""),
            )
        return JSONResponse({"ok": True, "recovered": recovered, "inventory": inventory.snapshot()})
    except InventoryError as exc:
        raise HTTPException(status_code=422, detail=str(exc)) from exc


@app.post("/api/inventory/reset-all")
async def inventory_reset_all(payload: dict[str, Any]) -> JSONResponse:
    if str(payload.get("confirm") or "") != "RESET_ALL_C_SLOTS":
        raise HTTPException(status_code=422, detail="confirm must be RESET_ALL_C_SLOTS")
    try:
        result = inventory.reset_all_available(str(payload.get("reason") or "operator inventory reset"))
    except InventoryError as exc:
        raise HTTPException(status_code=409, detail=str(exc)) from exc
    _reset_store_capture_session()
    gateway_state.add_log(
        "inventory-reset-all",
        f"reset {len(result['reset_slots'])} slots; next={result['next_store_slot']}",
        {"reason": result["reason"], "items": result["reset_items"]},
    )
    return JSONResponse({"ok": True, "reset": result, "inventory": inventory.snapshot()})


@app.get("/api/camera/snapshot.jpg")
def camera_snapshot() -> Response:
    path = camera_uploader.capture()
    if path is None:
        return JSONResponse({"ok": False, "error": gateway_state.camera_error}, status_code=503)
    return FileResponse(str(path), media_type="image/jpeg")


@app.get("/api/camera/stream.mjpg")
@app.get("/stream.mjpg")
@app.get("/video_feed")
def camera_stream() -> StreamingResponse:
    return StreamingResponse(
        camera_uploader.stream_mjpeg(),
        media_type="multipart/x-mixed-replace; boundary=frame",
    )


@app.post("/api/serial/result")
async def send_result(payload: dict[str, Any]) -> JSONResponse:
    status = int(payload.get("status", 0))
    style_id = int(payload.get("style_id", payload.get("styleId", 0)))
    ok = serial_link.write_bytes(result_frame(status, style_id), display=f"RESULT status={status} style={style_id}")
    return JSONResponse({"ok": ok})


@app.post("/api/serial/image-info")
async def send_image_info(payload: dict[str, Any]) -> JSONResponse:
    size = int(payload.get("size", 0))
    ok = serial_link.write_bytes(image_info_frame(size), display=f"IMAGE_INFO size={size}")
    return JSONResponse({"ok": ok})


@app.post("/api/serial/detection")
async def send_detection(payload: dict[str, Any]) -> JSONResponse:
    packet = detection_packet(
        people=payload.get("people", payload.get("persons", [])),
        cat_detected=payload.get("cat_detected", payload.get("cat", False)),
        dog_detected=payload.get("dog_detected", payload.get("dog", False)),
    )
    ok = serial_link.write_bytes(packet, display=f"DETECTION {packet.hex(' ')}")
    return JSONResponse({"ok": ok, "hex": packet.hex(" ")})


@app.post("/api/serial/reconnect")
def reconnect_serial() -> JSONResponse:
    serial_link.close()
    ok = serial_link.open()
    return JSONResponse({"ok": ok, "serial": gateway_state.snapshot()["serial"]})


@app.get("/api/status")
def status() -> JSONResponse:
    return JSONResponse(gateway_state.snapshot())


@app.get("/api/care/pressure")
def care_pressure() -> JSONResponse:
    return JSONResponse(gateway_state.care_gravity_snapshot(source="api"))


@app.get("/api/care/devices")
def care_devices() -> JSONResponse:
    devices = []
    for key, definition in CARE_DEVICE_DEFS.items():
        devices.append(
            {
                "id": key,
                "label": definition["label"],
                "supported": key != "ionizer",
                "note": "" if key != "ionizer" else "当前主板1固件未发现独立负离子输出，先作为预留计划项。",
            }
        )
    return JSONResponse({"devices": devices})


@app.get("/api/weather")
def weather() -> JSONResponse:
    try:
        snapshot = weather_client.fetch()
        weather_data = snapshot.public_dict()
        ok = True
        error = ""
    except Exception as exc:
        cached = weather_client.latest()
        weather_data = cached.public_dict() if cached is not None else None
        ok = weather_data is not None
        error = str(exc)
    humid = False
    if weather_data:
        cached_snapshot = weather_client.latest()
        humid = bool(cached_snapshot and is_humid_weather(
            cached_snapshot,
            config.weather_humidity_threshold,
            config.weather_precipitation_probability_threshold,
        ))
    return JSONResponse(
        {
            "ok": ok,
            "weather": weather_data,
            "humid": humid,
            "error": error,
            "auto_dehumidify": weather_care.status() if weather_care is not None else {
                "enabled": config.weather_enable,
                "humidity_threshold": config.weather_humidity_threshold,
                "precipitation_probability_threshold": config.weather_precipitation_probability_threshold,
                "duration_minutes": config.weather_care_duration_minutes,
            },
        }
    )


@app.get("/api/care/schedules")
def list_care_schedules() -> JSONResponse:
    return JSONResponse({"schedules": care_schedule_store.list()})


@app.put("/api/care/schedules")
async def replace_care_schedules(payload: dict[str, Any]) -> JSONResponse:
    schedules = payload.get("schedules", payload if isinstance(payload, list) else [])
    if not isinstance(schedules, list):
        raise HTTPException(status_code=422, detail="schedules must be a list")
    return JSONResponse({"schedules": care_schedule_store.replace(schedules)})


@app.post("/api/care/schedules")
async def upsert_care_schedule(payload: dict[str, Any]) -> JSONResponse:
    try:
        return JSONResponse({"schedule": care_schedule_store.update_one(payload)})
    except ValueError as exc:
        raise HTTPException(status_code=422, detail=str(exc)) from exc


@app.delete("/api/care/schedules/{schedule_id}")
def delete_care_schedule(schedule_id: str) -> JSONResponse:
    return JSONResponse({"ok": care_schedule_store.delete(schedule_id)})


@app.post("/api/care/schedules/run")
async def run_care_schedule(payload: dict[str, Any]) -> JSONResponse:
    if care_scheduler is None:
        raise HTTPException(status_code=503, detail="care scheduler is not running")
    try:
        schedule = care_scheduler.run_manual(payload)
        return JSONResponse({"ok": True, "schedule": schedule})
    except ValueError as exc:
        raise HTTPException(status_code=422, detail=str(exc)) from exc


@app.get("/api/serial/logs")
def serial_logs(limit: int = 100) -> JSONResponse:
    return JSONResponse({"logs": gateway_state.logs(limit)})


@app.api_route("/api/cloud/{path:path}", methods=["GET", "POST", "PUT", "PATCH", "DELETE"])
async def cloud_proxy(path: str, request: Request) -> Response:
    return await _proxy_cloud_api(path, request)


@app.get("/api/install-info")
def install_info() -> JSONResponse:
    return JSONResponse(
        {
            "serial_wiring": {
                "control_mode": "use board1 UART3_K230, 115200 8N1",
                "ch340_txd": "board1 PD9 / USART3_RX",
                "ch340_rxd": "board1 PD8 / USART3_TX",
                "ch340_gnd": "board1 GND",
                "vcc": "do not connect by default",
                "note": "PC6/PC7 USART6 is the old Raspberry Pi detection/screen link unless firmware is changed to parse @MCMD/@SCMD/@ACMD/@OCMD there.",
            },
            "serial_default": "/dev/serial/by-id/* then /dev/ttyUSB*",
            "service": "wardrobe-gateway.service",
        }
    )


@app.api_route("/api/{path:path}", methods=["GET", "POST", "PUT", "PATCH", "DELETE"])
async def cloud_api_fallback(path: str, request: Request) -> Response:
    return await _proxy_cloud_api(path, request)


async def _proxy_cloud_api(path: str, request: Request) -> Response:
    target = f"{config.cloud_api_base}/api/{path}"
    headers = {key: value for key, value in request.headers.items() if key.lower() not in {"host", "content-length"}}
    body = await request.body()
    try:
        upstream = requests.request(
            request.method,
            target,
            params=request.query_params,
            data=body if body else None,
            headers=headers,
            timeout=30,
        )
    except Exception as exc:
        return JSONResponse({"ok": False, "error": str(exc), "target": target}, status_code=502)
    return Response(
        content=upstream.content,
        status_code=upstream.status_code,
        media_type=upstream.headers.get("content-type"),
    )
