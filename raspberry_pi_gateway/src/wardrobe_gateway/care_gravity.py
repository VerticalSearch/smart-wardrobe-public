from __future__ import annotations

import re
from dataclasses import dataclass

GRAVITY_LINE_PREFIXES = ("@GRAV", "@GRAVITY", "@WEIGHT", "@WG", "@CARE")
LEFT_GRAVITY_KEYS = {"left", "l", "a", "ch1", "channel1", "channel_a", "gravity_left", "weight_left", "left_value"}
RIGHT_GRAVITY_KEYS = {"right", "r", "b", "ch2", "channel2", "channel_b", "gravity_right", "weight_right", "right_value"}
CARE_COUNTS_PREFIX = "@CARE"


@dataclass(frozen=True)
class CareGravityReading:
    left: float | None
    right: float | None
    unit: str


def parse_optional_float(value: str) -> float | None:
    try:
        number = float(value.strip())
    except (TypeError, ValueError):
        return None
    if number != number:
        return None
    return number


def parse_care_gravity_line(
    line: str,
) -> tuple[float | None, float | None] | None:
    reading = parse_care_gravity_reading(line)
    if reading is None:
        return None
    return reading.left, reading.right


def parse_care_gravity_reading(
    line: str,
) -> CareGravityReading | None:
    clean_line = line.strip()
    if not any(clean_line.upper().startswith(prefix) for prefix in GRAVITY_LINE_PREFIXES):
        return None
    is_care_counts = clean_line.upper().startswith(CARE_COUNTS_PREFIX)
    unit = "counts" if is_care_counts else "kg"

    payload = re.sub(r"^@[A-Za-z_]+", "", clean_line, count=1).strip("|,;: ")
    if not payload:
        return None

    left: float | None = None
    right: float | None = None
    positional: list[float] = []
    parts = [part.strip() for part in re.split(r"[|,;]", payload) if part.strip()]

    for part in parts:
        key = ""
        raw_value = part
        if "=" in part:
            key, raw_value = part.split("=", 1)
        elif ":" in part:
            key, raw_value = part.split(":", 1)

        number = parse_optional_float(raw_value)
        if number is None:
            lowered = part.lower()
            if lowered in LEFT_GRAVITY_KEYS or lowered in RIGHT_GRAVITY_KEYS:
                key = lowered
                continue
            continue

        normalized_key = key.strip().lower()
        if normalized_key in LEFT_GRAVITY_KEYS:
            left = number
        elif normalized_key in RIGHT_GRAVITY_KEYS:
            right = number
        else:
            positional.append(number)

    if positional:
        if left is None:
            left = positional[0]
        elif right is None:
            right = positional[0]
        if len(positional) > 1 and right is None:
            right = positional[1]

    if left is None and right is None:
        return None
    if is_care_counts:
        # Board1 already subtracts the HX711 tare offset before sending @CARE.
        # Keep these as raw net counts until a real weight calibration is made.
        pass
    return CareGravityReading(left=left, right=right, unit=unit)
