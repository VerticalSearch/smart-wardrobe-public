from wardrobe_gateway.protocol import (
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


def test_todo_maps_to_acmd():
    assert lines({"action": "todo", "action_id": 6, "id": "abc"}) == ["@ACMD|6|abc"]


def test_store_after_telescope_maps_to_acmd():
    assert lines({"action": "todo", "action_id": 11, "id": "store", "target_c": "C12"}) == ["@ACMD|11|store|12"]


def test_fetch_target_maps_to_acmd():
    assert lines({"action": "todo", "action_id": 7, "id": "fetch", "target_c": "C05"}) == ["@ACMD|7|fetch|5"]


def test_store_prepare_maps_to_acmd():
    assert lines({"action": "todo", "action_id": 12, "id": "prep"}) == ["@ACMD|12|prep"]


def test_store_retract_maps_to_acmd():
    assert lines({"action": "todo", "action_id": 13, "id": "retract"}) == ["@ACMD|13|retract"]


def test_fetch_reset_maps_to_acmd():
    assert lines({"action": "todo", "action_id": 14, "id": "reset"}) == ["@ACMD|14|reset"]


def test_raw_uart_cannot_bypass_inventory_actions():
    try:
        lines({"action": "raw_uart", "line": "@ACMD|7|legacy"})
    except ValueError as exc:
        assert "must use action=todo" in str(exc)
    else:
        raise AssertionError("raw inventory action must be rejected")


def test_emergency_sends_motion_and_action_stop():
    assert lines({"action": "emergency", "id": "stop"}) == [
        f"@MCMD|{CMD_EMERGENCY}|0|0|0|stop",
        "@ACMD|0|stop",
    ]


def test_stepper_enable_allows_six_channels():
    assert lines({"action": "stepper_enable", "channels": [1, 6], "enable": 1, "id": "en"}) == [
        f"@MCMD|{CMD_STEPPER_ENABLE}|1|1|0|en",
        f"@MCMD|{CMD_STEPPER_ENABLE}|6|1|0|en",
    ]


def test_stepper_relative_maps_direction():
    assert lines({"action": "stepper_move", "channel": 6, "mode": "relative", "dir": 1, "angle": 1000, "id": "mv"}) == [
        f"@MCMD|{CMD_STEPPER_ENABLE}|6|1|0|mv",
        f"@MCMD|{CMD_STEPPER_MOVE_ANGLE}|6|1|1000|mv",
    ]


def test_telescope_store_extend_and_retract_commands():
    assert lines({"action": "stepper_move", "channel": 4, "mode": "relative", "angle": -15500, "id": "ext"}) == [
        f"@MCMD|{CMD_STEPPER_ENABLE}|4|1|0|ext",
        f"@MCMD|{CMD_STEPPER_MOVE_ANGLE}|4|1|15500|ext",
    ]
    assert lines({"action": "stepper_move", "channel": 4, "mode": "relative", "angle": 15500, "id": "ret"}) == [
        f"@MCMD|{CMD_STEPPER_ENABLE}|4|1|0|ret",
        f"@MCMD|{CMD_STEPPER_MOVE_ANGLE}|4|0|15500|ret",
    ]


def test_stepper_absolute_maps_goto():
    assert lines({"action": "stepper_move", "channel": 2, "mode": "absolute", "angle": -120, "enable": False, "id": "go"}) == [
        f"@MCMD|{CMD_STEPPER_MOVE_ANGLE}|2|1|120|go",
    ]


def test_targets_include_axes_and_servos():
    assert lines({"action": "set_targets", "axes": {"1": 10, "6": -20}, "servos": {"1": 90}, "id": "t"}) == [
        f"@MCMD|{CMD_STEPPER_ENABLE}|1|1|0|t",
        f"@MCMD|{CMD_STEPPER_GOTO_ANGLE}|1|0|10|t",
        f"@MCMD|{CMD_STEPPER_ENABLE}|6|1|0|t",
        f"@MCMD|{CMD_STEPPER_GOTO_ANGLE}|6|0|-20|t",
        "@SCMD|1|90|t",
    ]


def test_camera_servo_channel_three_command():
    assert lines({"action": "servo", "channel": 3, "angle": 250, "id": "cam"}) == ["@SCMD|3|250|cam"]


def test_board1_output_maps_alias():
    assert lines({"action": "board1_output", "device": "fan", "enable": 1, "id": "o"}) == ["@OCMD|vent_fan|1|o"]


def test_result_frame_checksum():
    frame = result_frame(0, 5)
    assert frame == bytes([0xFF, 0x02, 0x00, 0x05, 0x00, 0xF8])


def test_detection_packet_shape():
    packet = detection_packet([{"x": 1, "y": 2, "area": 3}], cat_detected=True, dog_detected=False)
    assert packet == bytes([0xFF, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x01, 0x00, 0xFE])
