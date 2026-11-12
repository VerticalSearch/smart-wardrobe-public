from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))

from wardrobe_gateway.protocol import (  # noqa: E402
    CMD_EMERGENCY,
    CMD_STEPPER_ENABLE,
    CMD_STEPPER_GOTO_ANGLE,
    CMD_STEPPER_MOVE_ANGLE,
    build_serial_commands,
    detection_packet,
    result_frame,
)


def lines(payload):
    return [command.line for command in build_serial_commands(payload)]


def check(name: str, actual, expected) -> None:
    if actual != expected:
        raise AssertionError(f"{name}\nactual:   {actual!r}\nexpected: {expected!r}")


def main() -> None:
    check("todo", lines({"action": "todo", "action_id": 6, "id": "abc"}), ["@ACMD|6|abc"])
    check(
        "emergency",
        lines({"action": "emergency", "id": "stop"}),
        [f"@MCMD|{CMD_EMERGENCY}|0|0|0|stop", "@ACMD|0|stop"],
    )
    check(
        "stepper channels",
        lines({"action": "stepper_enable", "channels": [1, 6], "enable": 1, "id": "en"}),
        [f"@MCMD|{CMD_STEPPER_ENABLE}|1|1|0|en", f"@MCMD|{CMD_STEPPER_ENABLE}|6|1|0|en"],
    )
    check(
        "relative",
        lines({"action": "stepper_move", "channel": 6, "mode": "relative", "dir": 1, "angle": 1000, "id": "mv"}),
        [f"@MCMD|{CMD_STEPPER_ENABLE}|6|1|0|mv", f"@MCMD|{CMD_STEPPER_MOVE_ANGLE}|6|1|1000|mv"],
    )
    check(
        "targets",
        lines({"action": "set_targets", "axes": {"1": 10, "6": -20}, "servos": {"1": 90}, "id": "t"}),
        [
            f"@MCMD|{CMD_STEPPER_ENABLE}|1|1|0|t",
            f"@MCMD|{CMD_STEPPER_GOTO_ANGLE}|1|0|10|t",
            f"@MCMD|{CMD_STEPPER_ENABLE}|6|1|0|t",
            f"@MCMD|{CMD_STEPPER_GOTO_ANGLE}|6|0|-20|t",
            "@SCMD|1|90|t",
        ],
    )
    check("output", lines({"action": "board1_output", "device": "fan", "enable": 1, "id": "o"}), ["@OCMD|vent_fan|1|o"])
    check("result frame", result_frame(0, 5), bytes([0xFF, 0x02, 0x00, 0x05, 0x00, 0xF8]))
    check(
        "detection packet",
        detection_packet([{"x": 1, "y": 2, "area": 3}], cat_detected=True),
        bytes([0xFF, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x01, 0x00, 0xFE]),
    )
    print("protocol checks passed")


if __name__ == "__main__":
    main()

