from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path


def _bool_env(name: str, default: bool) -> bool:
    value = os.getenv(name)
    if value is None:
        return default
    return value.strip().lower() in {"1", "true", "yes", "on", "enable", "enabled"}


def _int_env(name: str, default: int) -> int:
    value = os.getenv(name)
    if value is None:
        return default
    try:
        return int(value.strip())
    except ValueError:
        return default


def _float_env(name: str, default: float) -> float:
    value = os.getenv(name)
    if value is None:
        return default
    try:
        return float(value.strip())
    except ValueError:
        return default


@dataclass(frozen=True)
class GatewayConfig:
    host: str
    port: int
    serial_port: str
    serial_baud: int
    serial_dry_run: bool
    upload_url: str
    upload_multi_url: str
    cloud_api_base: str
    device_token: str
    device_id: str
    mqtt_enable: bool
    mqtt_host: str
    mqtt_port: int
    mqtt_client_id: str
    mqtt_username: str
    mqtt_password: str
    mqtt_command_topic: str
    mqtt_status_topic: str
    mqtt_command_token: str
    camera_index: int
    camera_command: str
    camera_stream_command: str
    weather_enable: bool
    weather_latitude: float
    weather_longitude: float
    weather_location: str
    weather_humidity_threshold: int
    weather_precipitation_probability_threshold: int
    weather_care_duration_minutes: int
    weather_care_interval_seconds: int
    weather_care_cooldown_seconds: int
    capture_dir: Path
    display_html: Path
    project_root: Path

    @classmethod
    def from_env(cls) -> "GatewayConfig":
        package_root = Path(__file__).resolve().parents[2]
        project_root = package_root.parent
        cloud_base = os.getenv("WARDROBE_CLOUD_API_BASE", "http://119.45.230.229:8000").rstrip("/")
        return cls(
            host=os.getenv("WARDROBE_HOST", "0.0.0.0"),
            port=_int_env("WARDROBE_PORT", 8000),
            serial_port=os.getenv("WARDROBE_SERIAL_PORT", "auto"),
            serial_baud=_int_env("WARDROBE_SERIAL_BAUD", 115200),
            serial_dry_run=_bool_env("WARDROBE_SERIAL_DRY_RUN", False),
            upload_url=os.getenv("WARDROBE_UPLOAD_URL", f"{cloud_base}/api/wardrobe/add"),
            upload_multi_url=os.getenv("WARDROBE_UPLOAD_MULTI_URL", f"{cloud_base}/api/wardrobe/add-multi"),
            cloud_api_base=cloud_base,
            device_token=os.getenv("WARDROBE_DEVICE_TOKEN", ""),
            device_id=os.getenv("WARDROBE_DEVICE_ID", "raspberry-pi-wardrobe-001"),
            mqtt_enable=_bool_env("WARDROBE_MQTT_ENABLE", True),
            mqtt_host=os.getenv("WARDROBE_MQTT_HOST", "broker.emqx.io"),
            mqtt_port=_int_env("WARDROBE_MQTT_PORT", 1883),
            mqtt_client_id=os.getenv("WARDROBE_MQTT_CLIENT_ID", "raspberry-pi-wardrobe-001"),
            mqtt_username=os.getenv("WARDROBE_MQTT_USER", ""),
            mqtt_password=os.getenv("WARDROBE_MQTT_PASSWORD", ""),
            mqtt_command_topic=os.getenv("WARDROBE_MQTT_COMMAND_TOPIC", "smart_wardrobe/raspberry-pi-wardrobe-001/cmd"),
            mqtt_status_topic=os.getenv("WARDROBE_MQTT_STATUS_TOPIC", "smart_wardrobe/raspberry-pi-wardrobe-001/status"),
            mqtt_command_token=os.getenv("WARDROBE_MQTT_COMMAND_TOKEN", ""),
            camera_index=_int_env("WARDROBE_CAMERA_INDEX", 0),
            camera_command=os.getenv("WARDROBE_CAMERA_COMMAND", ""),
            camera_stream_command=os.getenv("WARDROBE_CAMERA_STREAM_COMMAND", ""),
            weather_enable=_bool_env("WARDROBE_WEATHER_ENABLE", True),
            weather_latitude=_float_env("WARDROBE_WEATHER_LATITUDE", 33.35),
            weather_longitude=_float_env("WARDROBE_WEATHER_LONGITUDE", 120.16),
            weather_location=os.getenv("WARDROBE_WEATHER_LOCATION", "盐城"),
            weather_humidity_threshold=_int_env("WARDROBE_WEATHER_HUMIDITY_THRESHOLD", 75),
            weather_precipitation_probability_threshold=_int_env("WARDROBE_WEATHER_RAIN_PROBABILITY_THRESHOLD", 60),
            weather_care_duration_minutes=_int_env("WARDROBE_WEATHER_CARE_DURATION_MINUTES", 15),
            weather_care_interval_seconds=_int_env("WARDROBE_WEATHER_CARE_INTERVAL_SECONDS", 900),
            weather_care_cooldown_seconds=_int_env("WARDROBE_WEATHER_CARE_COOLDOWN_SECONDS", 21600),
            capture_dir=Path(os.getenv("WARDROBE_CAPTURE_DIR", str(package_root / "captures"))),
            display_html=Path(
                os.getenv(
                    "WARDROBE_DISPLAY_HTML",
                    str(project_root / "02_界面原型" / "cloud_display.html"),
                )
            ),
            project_root=project_root,
        )
