from __future__ import annotations

import base64
import subprocess
import time
from pathlib import Path
from typing import Any

import requests

from .config import GatewayConfig
from .protocol import STATUS_CAM_FAIL, STATUS_NO_RESULT, STATUS_OK, STATUS_UPLOAD_FAIL, STYLE_UNKNOWN, clamp_int
from .state import GatewayState


STYLE_KEYWORDS = (
    ("t-shirt", 0x01),
    ("tshirt", 0x01),
    ("tee", 0x01),
    ("短袖", 0x01),
    ("t恤", 0x01),
    ("shirt", 0x02),
    ("衬衫", 0x02),
    ("coat", 0x03),
    ("外套", 0x03),
    ("大衣", 0x03),
    ("dress", 0x04),
    ("连衣裙", 0x04),
    ("pants", 0x05),
    ("trousers", 0x05),
    ("jeans", 0x05),
    ("裤", 0x05),
    ("skirt", 0x06),
    ("裙", 0x06),
    ("shorts", 0x07),
    ("短裤", 0x07),
    ("sweater", 0x08),
    ("毛衣", 0x08),
    ("hoodie", 0x09),
    ("卫衣", 0x09),
    ("jacket", 0x0A),
    ("夹克", 0x0A),
    ("shoe", 0x0B),
    ("sneaker", 0x0B),
    ("鞋", 0x0B),
)

UPLOAD_RESPONSE_TIMEOUT_SECONDS = 60
LATE_UPLOAD_POLL_SECONDS = 20.0
LATE_UPLOAD_POLL_INTERVAL_SECONDS = 1.0
LATE_UPLOAD_REQUEST_TIMEOUT_SECONDS = 5


def style_id_from_response(data: Any) -> int:
    if isinstance(data, dict):
        for key in ("style_id", "styleId", "category_id", "categoryId", "class_id", "type_id"):
            value = data.get(key)
            if value is not None:
                style = clamp_int(value, STYLE_UNKNOWN, 0, 255)
                if style:
                    return style
        for key in ("style", "category", "type", "name", "label", "result", "message", "desc"):
            style = style_id_from_response(data.get(key))
            if style:
                return style
        for value in data.values():
            style = style_id_from_response(value)
            if style:
                return style
    elif isinstance(data, list):
        for item in data:
            style = style_id_from_response(item)
            if style:
                return style
    elif data is not None:
        text = str(data).lower()
        for keyword, style in STYLE_KEYWORDS:
            if keyword in text:
                return style
    return STYLE_UNKNOWN


class CameraUploader:
    def __init__(self, config: GatewayConfig, state: GatewayState) -> None:
        self.config = config
        self.state = state
        self._stream_proc: subprocess.Popen[bytes] | None = None
        self.config.capture_dir.mkdir(parents=True, exist_ok=True)

    def capture_upload(self) -> tuple[int, int, dict[str, Any]]:
        path = self.capture()
        if path is None:
            return STATUS_CAM_FAIL, STYLE_UNKNOWN, {"ok": False, "error": self.state.camera_error}
        return self.upload(path)

    def compose_vertical(self, paths: list[Path], *, label: str = "multi_angle") -> Path | None:
        valid_paths = [path for path in paths if path.exists() and path.stat().st_size > 0]
        if not valid_paths:
            return None
        if len(valid_paths) == 1:
            return valid_paths[0]

        output = self.config.capture_dir / f"{label}_{time.time_ns()}.jpg"
        inputs: list[str] = []
        filters: list[str] = []
        stack_inputs: list[str] = []
        for index, path in enumerate(valid_paths):
            inputs.extend(["-i", str(path)])
            filters.append(
                f"[{index}:v]scale=1280:720:force_original_aspect_ratio=decrease,"
                f"pad=1280:720:(ow-iw)/2:(oh-ih)/2[v{index}]"
            )
            stack_inputs.append(f"[v{index}]")
        filter_complex = ";".join(filters + [f"{''.join(stack_inputs)}vstack=inputs={len(valid_paths)}[out]"])
        command = [
            "ffmpeg",
            "-y",
            "-hide_banner",
            "-loglevel",
            "error",
            *inputs,
            "-filter_complex",
            filter_complex,
            "-map",
            "[out]",
            "-q:v",
            "4",
            str(output),
        ]
        ok, error = self._run_command(command)
        if ok and output.exists() and output.stat().st_size > 0:
            self.state.add_log(
                "camera-compose",
                f"composed {len(valid_paths)} frames",
                {"path": str(output), "size": output.stat().st_size},
            )
            return output
        self.state.add_log("camera-compose-error", error or "empty output", {"frames": len(valid_paths)})
        output.unlink(missing_ok=True)
        return None

    def stream_mjpeg(self):
        self._stop_stream_proc()
        command = self.config.camera_stream_command.format(index=self.config.camera_index)
        shell = bool(command)
        if not command:
            command = [
                "ffmpeg",
                "-hide_banner",
                "-loglevel",
                "error",
                "-f",
                "v4l2",
                "-input_format",
                "mjpeg",
                "-video_size",
                "1280x720",
                "-framerate",
                "15",
                "-i",
                f"/dev/video{self.config.camera_index}",
                "-an",
                "-c:v",
                "copy",
                "-f",
                "image2pipe",
                "pipe:1",
            ]
        proc: subprocess.Popen[bytes] | None = None
        try:
            proc = subprocess.Popen(
                command,
                shell=shell,
                stdout=subprocess.PIPE,
                stderr=subprocess.DEVNULL,
                stdin=subprocess.DEVNULL,
                bufsize=0,
            )
            self._stream_proc = proc
            self.state.camera_ok = True
            self.state.camera_error = ""
            self.state.add_log("camera-stream", "started mjpeg stream")
            if proc.stdout is None:
                raise RuntimeError("ffmpeg stdout is unavailable")
            buffer = bytearray()
            while True:
                chunk = proc.stdout.read(4096)
                if not chunk:
                    break
                buffer.extend(chunk)
                while True:
                    start = buffer.find(b"\xff\xd8")
                    if start < 0:
                        if len(buffer) > 4096:
                            del buffer[:-2]
                        break
                    end = buffer.find(b"\xff\xd9", start + 2)
                    if end < 0:
                        if start > 0:
                            del buffer[:start]
                        break
                    frame = bytes(buffer[start : end + 2])
                    del buffer[: end + 2]
                    yield (
                        b"--frame\r\n"
                        b"Content-Type: image/jpeg\r\n"
                        + f"Content-Length: {len(frame)}\r\n\r\n".encode("ascii")
                        + frame
                        + b"\r\n"
                    )
        except GeneratorExit:
            raise
        except Exception as exc:
            self.state.camera_ok = False
            self.state.camera_error = str(exc)
            self.state.add_log("camera-stream-error", str(exc))
            raise
        finally:
            if proc is not None and proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    proc.kill()
            if self._stream_proc is proc:
                self._stream_proc = None
            self.state.add_log("camera-stream", "stopped mjpeg stream")

    def _stop_stream_proc(self) -> None:
        proc = self._stream_proc
        if proc is None or proc.poll() is not None:
            self._stream_proc = None
            return
        proc.terminate()
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            proc.kill()
        self._stream_proc = None

    def capture(self) -> Path | None:
        path = self.config.capture_dir / f"capture_{time.time_ns()}.jpg"
        try:
            errors: list[str] = []
            for label, command, shell in self._capture_commands(path):
                path.unlink(missing_ok=True)
                ok, error = self._run_command(command, shell=shell)
                if ok and path.exists() and path.stat().st_size > 0:
                    break
                errors.append(f"{label}: {error or 'empty output'}")
            else:
                detail = "; ".join(errors[:6]) if errors else "no capture commands configured"
                raise RuntimeError(f"no supported camera command succeeded ({detail})")

            self.state.camera_ok = True
            self.state.camera_error = ""
            self.state.last_capture = {"ok": True, "path": str(path), "size": path.stat().st_size}
            self.state.add_log("camera", f"captured {path.name}", {"size": path.stat().st_size})
            return path
        except Exception as exc:
            self.state.camera_ok = False
            self.state.camera_error = str(exc)
            self.state.last_capture = {"ok": False, "error": str(exc)}
            self.state.add_log("camera-error", str(exc))
            return None

    def _capture_commands(self, path: Path) -> list[tuple[str, list[str] | str, bool]]:
        video_device = f"/dev/video{self.config.camera_index}"
        commands: list[tuple[str, list[str] | str, bool]] = []
        if self.config.camera_command:
            commands.append((
                "configured",
                self.config.camera_command.format(path=str(path), index=self.config.camera_index),
                True,
            ))
        commands.extend([
            (
                "rpicam-still",
                ["rpicam-still", "-n", "-o", str(path), "--width", "1280", "--height", "720", "--timeout", "1000"],
                False,
            ),
            (
                "libcamera-still",
                ["libcamera-still", "-n", "-o", str(path), "--width", "1280", "--height", "720", "--timeout", "1000"],
                False,
            ),
            (
                "ffmpeg-v4l2-mjpeg-1280x720",
                [
                    "ffmpeg",
                    "-y",
                    "-hide_banner",
                    "-loglevel",
                    "error",
                    "-f",
                    "v4l2",
                    "-input_format",
                    "mjpeg",
                    "-video_size",
                    "1280x720",
                    "-i",
                    video_device,
                    "-frames:v",
                    "1",
                    str(path),
                ],
                False,
            ),
            (
                "ffmpeg-v4l2-auto",
                [
                    "ffmpeg",
                    "-y",
                    "-hide_banner",
                    "-loglevel",
                    "error",
                    "-f",
                    "v4l2",
                    "-i",
                    video_device,
                    "-frames:v",
                    "1",
                    str(path),
                ],
                False,
            ),
            (
                "fswebcam",
                ["fswebcam", "-d", video_device, "-r", "1280x720", "--jpeg", "85", str(path)],
                False,
            ),
        ])
        return commands

    @staticmethod
    def _run_command(command: list[str] | str, *, shell: bool = False) -> tuple[bool, str]:
        try:
            result = subprocess.run(
                command,
                shell=shell,
                check=True,
                timeout=20,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            output = (result.stderr or result.stdout or "").strip()
            return True, output[:240]
        except FileNotFoundError as exc:
            return False, f"not found: {exc.filename}"
        except subprocess.CalledProcessError as exc:
            output = (exc.stderr or exc.stdout or "").strip()
            return False, output[:240] or f"exit {exc.returncode}"
        except subprocess.TimeoutExpired:
            return False, "timeout"

    @staticmethod
    def _same_position(left: Any, right: Any) -> bool:
        def normalize(value: Any) -> str:
            text = str(value or "").strip().upper()
            if text.startswith("C"):
                text = text[1:]
            try:
                return f"C{int(text):02d}"
            except (TypeError, ValueError):
                return ""

        normalized_left = normalize(left)
        return bool(normalized_left) and normalized_left == normalize(right)

    def _poll_late_upload_item(self, metadata: dict[str, Any]) -> tuple[dict[str, Any] | None, dict[str, Any]]:
        command_id = str(metadata.get("capture_session_id") or "").strip()
        real_position = str(metadata.get("real_position") or "").strip()
        detail: dict[str, Any] = {"attempts": 0, "errors": []}
        if not command_id or not real_position:
            detail["error"] = "late upload lookup requires capture_session_id and real_position"
            return None, detail

        cloud_base = str(getattr(self.config, "cloud_api_base", "") or "").rstrip("/")
        if not cloud_base:
            upload_url = str(getattr(self.config, "upload_multi_url", "") or "")
            cloud_base = upload_url.split("/api/", 1)[0].rstrip("/")
        if not cloud_base:
            detail["error"] = "cloud api base is unavailable"
            return None, detail

        headers: dict[str, str] = {}
        if getattr(self.config, "device_token", ""):
            headers["Authorization"] = f"Bearer {self.config.device_token}"
        deadline = time.monotonic() + max(0.0, float(LATE_UPLOAD_POLL_SECONDS))
        while True:
            detail["attempts"] += 1
            try:
                response = requests.get(
                    f"{cloud_base}/api/wardrobe",
                    headers=headers,
                    timeout=LATE_UPLOAD_REQUEST_TIMEOUT_SECONDS,
                )
                if 200 <= response.status_code < 300:
                    body = response.json()
                    matches = []
                    if isinstance(body, list):
                        matches = [
                            item
                            for item in body
                            if isinstance(item, dict)
                            and str(item.get("capture_session_id") or "").strip() == command_id
                            and self._same_position(item.get("real_position"), real_position)
                            and str(item.get("id") or "").strip()
                        ]
                    if len(matches) == 1:
                        detail["matched_item_id"] = str(matches[0].get("id") or "")
                        return matches[0], detail
                    if len(matches) > 1:
                        detail["error"] = "multiple cloud items matched the same capture session and C slot"
                        detail["matched_item_ids"] = [str(item.get("id") or "") for item in matches]
                        return None, detail
                else:
                    detail["errors"].append(f"HTTP {response.status_code}: {response.text[:120]}")
            except Exception as exc:
                detail["errors"].append(str(exc))

            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            time.sleep(min(max(0.0, float(LATE_UPLOAD_POLL_INTERVAL_SECONDS)), remaining))
        detail.setdefault("error", "no unique late cloud item appeared before the reconciliation deadline")
        return None, detail

    def upload(
        self,
        path: Path,
        cloud_metadata: dict[str, Any] | None = None,
    ) -> tuple[int, int, dict[str, Any]]:
        try:
            image_b64 = "data:image/jpeg;base64," + base64.b64encode(path.read_bytes()).decode("ascii")
            metadata = {
                key: value
                for key, value in dict(cloud_metadata or {}).items()
                if key in {"capture_session_id", "source", "position", "real_position", "real_position_index"}
                and value not in (None, "")
            }
            if not metadata.get("real_position"):
                result = {
                    "ok": False,
                    "error": "wardrobe upload requires real_position from an active inventory transaction",
                    "path": str(path),
                    "cloud_metadata": metadata,
                }
                self.state.last_upload = result
                self.state.add_log("upload-blocked", result["error"])
                return STATUS_UPLOAD_FAIL, STYLE_UNKNOWN, result
            payload = {"images_base64": [image_b64], **metadata}
            upload_url = self.config.upload_multi_url
            headers = {"Content-Type": "application/json"}
            if self.config.device_token:
                headers["Authorization"] = f"Bearer {self.config.device_token}"
            response = requests.post(
                upload_url,
                json=payload,
                headers=headers,
                timeout=UPLOAD_RESPONSE_TIMEOUT_SECONDS,
            )
            text = response.text
            try:
                body: Any = response.json()
            except Exception:
                body = text

            upload_ok = False
            item_id = ""
            if isinstance(body, dict):
                item = body.get("item")
                if isinstance(item, dict):
                    item_id = str(item.get("id") or "")
            if 200 <= response.status_code < 300:
                style_id = style_id_from_response(body)
                upload_ok = True
                if isinstance(body, dict) and body.get("item"):
                    status = STATUS_OK
                else:
                    status = STATUS_OK if style_id != STYLE_UNKNOWN else STATUS_NO_RESULT
            else:
                style_id = STYLE_UNKNOWN
                status = STATUS_UPLOAD_FAIL

            result = {
                "ok": upload_ok and status == STATUS_OK,
                "status_code": response.status_code,
                "style_id": style_id,
                "path": str(path),
                "item_id": item_id,
                "cloud_metadata": metadata,
                "upload_url": upload_url,
                "response": body if isinstance(body, (dict, list)) else str(body)[:300],
            }
            self.state.last_upload = result
            self.state.add_log("upload", f"status={status}, style={style_id}", {"http": response.status_code})
            return status, style_id, result
        except requests.ReadTimeout as exc:
            metadata = {
                key: value
                for key, value in dict(cloud_metadata or {}).items()
                if key in {"capture_session_id", "source", "position", "real_position", "real_position_index"}
                and value not in (None, "")
            }
            item, reconciliation = self._poll_late_upload_item(metadata)
            if item is not None:
                style_id = style_id_from_response(item)
                result = {
                    "ok": True,
                    "status_code": 200,
                    "style_id": style_id,
                    "path": str(path),
                    "item_id": str(item.get("id") or ""),
                    "cloud_metadata": metadata,
                    "upload_url": str(getattr(self.config, "upload_multi_url", "")),
                    "response": {"item": item},
                    "late_resolved": True,
                    "original_error": str(exc),
                    "late_reconciliation": reconciliation,
                }
                self.state.last_upload = result
                self.state.add_log(
                    "upload-late-resolved",
                    f"{metadata.get('capture_session_id')} -> {result['item_id']}",
                    {"real_position": metadata.get("real_position"), "attempts": reconciliation.get("attempts")},
                )
                return STATUS_OK, style_id, result
            result = {
                "ok": False,
                "error": str(exc),
                "path": str(path),
                "cloud_metadata": metadata,
                "late_reconciliation": reconciliation,
            }
            self.state.last_upload = result
            self.state.add_log("upload-timeout-unresolved", str(exc), reconciliation)
            return STATUS_UPLOAD_FAIL, STYLE_UNKNOWN, result
        except Exception as exc:
            result = {"ok": False, "error": str(exc), "path": str(path)}
            self.state.last_upload = result
            self.state.add_log("upload-error", str(exc))
            return STATUS_UPLOAD_FAIL, STYLE_UNKNOWN, result
