from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Any


CMD_EMERGENCY = 0x03
CMD_STEPPER_ENABLE = 0x30
CMD_STEPPER_MOVE_ANGLE = 0x31
CMD_STEPPER_GOTO_ANGLE = 0x32

FRAME_HEADER = 0xFF
CMD_CAPTURE = 0x01
CMD_RESULT = 0x02
CMD_IMAGE_INFO = 0x03
CMD_IMAGE_READY = 0x04

STATUS_OK = 0x00
STATUS_CAM_FAIL = 0x01
STATUS_WIFI_FAIL = 0x02
STATUS_UPLOAD_FAIL = 0x03
STATUS_NO_RESULT = 0x04

STYLE_UNKNOWN = 0x00

RAW_UART_ALLOWED_PREFIXES = ("@MCMD|", "@SCMD|", "@ACMD|", "@OCMD|")


@dataclass(frozen=True)
class SerialCommand:
    line: str
    kind: str
    detail: str = ""


def checksum(data: bytes | bytearray) -> int:
    value = 0
    for item in data:
        value ^= item
    return value & 0xFF


def result_frame(status: int, style_id: int = STYLE_UNKNOWN) -> bytes:
    frame = bytearray(6)
    frame[0] = FRAME_HEADER
    frame[1] = CMD_RESULT
    frame[2] = clamp_int(status, STATUS_UPLOAD_FAIL, 0, 255)
    frame[3] = clamp_int(style_id, STYLE_UNKNOWN, 0, 255)
    frame[4] = 0
    frame[5] = checksum(frame[:-1])
    return bytes(frame)


def image_info_frame(size: int) -> bytes:
    frame = bytearray(7)
    frame[0] = FRAME_HEADER
    frame[1] = CMD_IMAGE_INFO
    size = clamp_int(size, 0, 0, 0xFFFFFFFF)
    frame[2] = (size >> 24) & 0xFF
    frame[3] = (size >> 16) & 0xFF
    frame[4] = (size >> 8) & 0xFF
    frame[5] = size & 0xFF
    frame[6] = checksum(frame[:-1])
    return bytes(frame)


def detection_packet(
    people: list[dict[str, Any]] | None = None,
    cat_detected: bool | int = False,
    dog_detected: bool | int = False,
) -> bytes:
    # The current board1 Raspberry_Pi DMA buffer is 32 bytes, so keep at most 4 people.
    clean_people = list(people or [])[:4]
    frame = bytearray()
    frame.append(0xFF)
    frame.extend(len(clean_people).to_bytes(2, "big"))
    for person in clean_people:
        frame.extend(clamp_int(person.get("x"), 0, 0, 65535).to_bytes(2, "big"))
        frame.extend(clamp_int(person.get("y"), 0, 0, 65535).to_bytes(2, "big"))
        frame.extend(clamp_int(person.get("area"), 0, 0, 65535).to_bytes(2, "big"))
    frame.append(1 if cat_detected else 0)
    frame.append(1 if dog_detected else 0)
    frame.append(0xFE)
    return bytes(frame)


def clamp_int(value: Any, default: int, min_value: int, max_value: int) -> int:
    try:
        if isinstance(value, bool):
            number = int(value)
        elif isinstance(value, str):
            number = int(float(value.strip() or str(default)))
        else:
            number = int(value)
    except (TypeError, ValueError, OverflowError):
        number = default
    return max(min_value, min(max_value, number))


def clean_text(value: Any, max_len: int = 24) -> str:
    text = str(value or "").strip()
    cleaned = []
    for ch in text:
        if ch in {"|", "\r", "\n"}:
            ch = " "
        if ord(ch) < 32:
            continue
        cleaned.append(ch)
        if len(cleaned) >= max_len:
            break
    return "".join(cleaned)


def clean_uart_line(value: Any, max_len: int = 160) -> str:
    text = str(value or "").strip()
    cleaned = []
    for ch in text:
        code = ord(ch)
        if ch in {"\r", "\n"} or code < 32 or code > 126:
            continue
        cleaned.append(ch)
        if len(cleaned) >= max_len:
            break
    return "".join(cleaned)


def clean_device_token(value: Any, max_len: int = 16) -> str:
    text = str(value or "").strip().lower().replace(" ", "_")
    return "".join(re.findall(r"[a-z0-9_-]", text))[:max_len]


def normalize_output_device(value: Any) -> str:
    token = clean_device_token(value)
    if token in {"uv", "uv_lamp", "ultraviolet", "dirc", "1"}:
        return "uv_lamp"
    if token in {"fan", "vent", "vent_fan", "pwmb", "2"}:
        return "vent_fan"
    return token


def action_from_payload(payload: dict[str, Any]) -> str:
    for key in ("action", "cmd", "type"):
        if key in payload:
            return str(payload.get(key, "")).strip().lower()
    return ""


def map_get(data: Any, key: int, default: Any = None) -> Any:
    if not isinstance(data, dict):
        return default
    if key in data:
        return data[key]
    return data.get(str(key), default)


def _motion_angle_text(cmd: int, channel: int, angle: Any) -> str:
    if cmd == CMD_STEPPER_GOTO_ANGLE:
        if channel == 6:
            return str(clamp_int(angle, 0, -9_223_372_036_854_775_807, 9_223_372_036_854_775_807))
        return str(clamp_int(angle, 0, -32768, 32767))
    return str(abs(clamp_int(angle, 0, -65535, 65535)))


def motion_line(cmd: int, channel: int, control: int = 0, angle: Any = 0, command_id: Any = "") -> SerialCommand:
    cmd_value = clamp_int(cmd, 0, 0, 255)
    channel_value = clamp_int(channel, 1, 0, 255)
    control_value = clamp_int(control, 0, 0, 255)
    angle_text = _motion_angle_text(cmd_value, channel_value, angle)
    line = f"@MCMD|{cmd_value}|{channel_value}|{control_value}|{angle_text}|{clean_text(command_id)}"
    return SerialCommand(line=line, kind="motion")


def servo_line(channel: int, angle: Any, command_id: Any = "") -> SerialCommand:
    line = f"@SCMD|{clamp_int(channel, 1, 1, 4)}|{clamp_int(angle, 135, 0, 270)}|{clean_text(command_id)}"
    return SerialCommand(line=line, kind="servo")


def target_c_number(value: Any) -> int:
    if isinstance(value, bool):
        raise ValueError("target_c must be C01-C12")
    text = str(value or "").strip().upper()
    if text.startswith("C"):
        text = text[1:]
    try:
        number = int(text)
    except (TypeError, ValueError) as exc:
        raise ValueError("target_c must be C01-C12") from exc
    if number < 1 or number > 12:
        raise ValueError("target_c must be C01-C12")
    return number


def action_line(action_id: Any, command_id: Any = "", target_c: Any = None) -> SerialCommand:
    line = f"@ACMD|{clamp_int(action_id, 6, 0, 255)}|{clean_text(command_id)}"
    if target_c not in (None, "", 0, "0"):
        line += f"|{target_c_number(target_c)}"
    return SerialCommand(line=line, kind="action")


def output_line(device: Any, enable: Any, command_id: Any = "") -> SerialCommand:
    device_name = normalize_output_device(device)
    if not device_name:
        raise ValueError("missing board1 output device")
    line = f"@OCMD|{device_name}|{clamp_int(enable, 0, 0, 1)}|{clean_text(command_id)}"
    return SerialCommand(line=line, kind="output")


def build_serial_commands(payload: dict[str, Any]) -> list[SerialCommand]:
    if not isinstance(payload, dict):
        raise ValueError("control payload must be a JSON object")

    action = action_from_payload(payload)
    command_id = payload.get("id", "")
    commands: list[SerialCommand] = []

    if action in {"", "status", "capture"}:
        return commands

    if action == "raw_uart":
        line = clean_uart_line(payload.get("line", ""))
        if not any(line.startswith(prefix) for prefix in RAW_UART_ALLOWED_PREFIXES):
            raise ValueError("raw_uart line must start with @MCMD, @SCMD, @ACMD, or @OCMD")
        if line.startswith("@ACMD|"):
            parts = line.split("|")
            try:
                raw_action_id = int(parts[1])
            except (IndexError, ValueError) as exc:
                raise ValueError("invalid raw @ACMD action id") from exc
            if raw_action_id in {6, 7, 11}:
                raise ValueError("inventory actions 6, 7, and 11 must use action=todo")
        return [SerialCommand(line=line, kind="raw")]

    if action == "stepper_enable":
        enable = clamp_int(payload.get("enable", payload.get("control", 1)), 1, 0, 1)
        channels = payload.get("channels")
        if isinstance(channels, list):
            for item in channels:
                commands.append(motion_line(CMD_STEPPER_ENABLE, clamp_int(item, 1, 1, 6), enable, 0, command_id))
        else:
            channel = clamp_int(payload.get("channel", payload.get("axis", 1)), 1, 1, 6)
            commands.append(motion_line(CMD_STEPPER_ENABLE, channel, enable, 0, command_id))
        return commands

    if action == "stepper_move":
        channel = clamp_int(payload.get("channel", payload.get("axis", 1)), 1, 1, 6)
        raw_angle = payload.get("angle", payload.get("angle_deg", 0))
        angle = clamp_int(raw_angle, 0, -65535, 65535)
        if payload.get("enable", True):
            commands.append(motion_line(CMD_STEPPER_ENABLE, channel, 1, 0, command_id))
        relative = bool(payload.get("relative", False)) or str(payload.get("mode", "")).lower() == "relative"
        if angle < 0:
            commands.append(motion_line(CMD_STEPPER_MOVE_ANGLE, channel, 1, -angle, command_id))
        elif relative:
            direction = clamp_int(payload.get("dir", payload.get("direction", 0)), 0, 0, 1)
            commands.append(motion_line(CMD_STEPPER_MOVE_ANGLE, channel, direction, angle, command_id))
        else:
            commands.append(motion_line(CMD_STEPPER_GOTO_ANGLE, channel, 0, raw_angle, command_id))
        return commands

    if action == "set_targets":
        axes = payload.get("axes", payload.get("targets"))
        if isinstance(axes, dict):
            for channel in range(1, 7):
                value = map_get(axes, channel)
                if value is None:
                    continue
                if payload.get("enable", True):
                    commands.append(motion_line(CMD_STEPPER_ENABLE, channel, 1, 0, command_id))
                commands.append(motion_line(CMD_STEPPER_GOTO_ANGLE, channel, 0, value, command_id))
        servos = payload.get("servos")
        if isinstance(servos, dict):
            for channel in range(1, 5):
                value = map_get(servos, channel)
                if value is not None:
                    commands.append(servo_line(channel, value, command_id))
        return commands

    if action == "servo":
        return [servo_line(payload.get("channel", 1), payload.get("angle", payload.get("angle_deg", 135)), command_id)]

    if action == "todo":
        return [
            action_line(
                payload.get("action_id", payload.get("value", 6)),
                command_id,
                payload.get("target_c", payload.get("real_position")),
            )
        ]

    if action == "emergency":
        return [
            motion_line(CMD_EMERGENCY, 0, 0, 0, command_id),
            action_line(0, command_id),
        ]

    if action == "board1_output":
        return [output_line(payload.get("device", payload.get("channel", "")), payload.get("enable", payload.get("value", 0)), command_id)]

    raise ValueError(f"unknown control action: {action}")
