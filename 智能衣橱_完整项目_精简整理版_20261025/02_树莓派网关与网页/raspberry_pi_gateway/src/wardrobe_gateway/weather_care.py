from __future__ import annotations

import threading
import time
import json
import urllib.parse
import urllib.request
from dataclasses import dataclass
from typing import Any, Callable


CommandSender = Callable[[dict[str, Any], str], dict[str, Any]]


WEATHER_CODE_TEXT = {
    0: "晴",
    1: "少云",
    2: "多云",
    3: "阴",
    45: "雾",
    48: "霜雾",
    51: "小毛毛雨",
    53: "毛毛雨",
    55: "大毛毛雨",
    61: "小雨",
    63: "中雨",
    65: "大雨",
    80: "阵雨",
    81: "阵雨",
    82: "强阵雨",
    95: "雷雨",
}


@dataclass(frozen=True)
class WeatherSnapshot:
    location: str
    latitude: float
    longitude: float
    temperature_c: float | None
    humidity_percent: int | None
    precipitation_mm: float | None
    rain_mm: float | None
    precipitation_probability: int | None
    weather_code: int | None
    observed_at: str
    fetched_at: float

    def public_dict(self) -> dict[str, Any]:
        condition = WEATHER_CODE_TEXT.get(self.weather_code or -1, "未知")
        wet = is_humid_weather(self)
        parts = [self.location, condition]
        if self.temperature_c is not None:
            parts.append(f"{self.temperature_c:.1f}C")
        if self.humidity_percent is not None:
            parts.append(f"湿度{self.humidity_percent}%")
        if self.precipitation_probability is not None:
            parts.append(f"降水{self.precipitation_probability}%")
        if wet:
            parts.append("偏潮湿")
        return {
            "location": self.location,
            "latitude": self.latitude,
            "longitude": self.longitude,
            "temperature_c": self.temperature_c,
            "humidity_percent": self.humidity_percent,
            "precipitation_mm": self.precipitation_mm,
            "rain_mm": self.rain_mm,
            "precipitation_probability": self.precipitation_probability,
            "weather_code": self.weather_code,
            "condition": condition,
            "humid": wet,
            "summary": " · ".join(parts),
            "observed_at": self.observed_at,
            "fetched_at": self.fetched_at,
        }


def _float_or_none(value: Any) -> float | None:
    try:
        return float(value)
    except (TypeError, ValueError, OverflowError):
        return None


def _int_or_none(value: Any) -> int | None:
    try:
        return int(round(float(value)))
    except (TypeError, ValueError, OverflowError):
        return None


def parse_open_meteo(data: dict[str, Any], *, location: str, latitude: float, longitude: float, fetched_at: float | None = None) -> WeatherSnapshot:
    current = data.get("current") if isinstance(data.get("current"), dict) else {}
    hourly = data.get("hourly") if isinstance(data.get("hourly"), dict) else {}
    probability = None
    probabilities = hourly.get("precipitation_probability")
    if isinstance(probabilities, list) and probabilities:
        probability = _int_or_none(probabilities[0])
    return WeatherSnapshot(
        location=location,
        latitude=latitude,
        longitude=longitude,
        temperature_c=_float_or_none(current.get("temperature_2m")),
        humidity_percent=_int_or_none(current.get("relative_humidity_2m")),
        precipitation_mm=_float_or_none(current.get("precipitation")),
        rain_mm=_float_or_none(current.get("rain")),
        precipitation_probability=probability,
        weather_code=_int_or_none(current.get("weather_code")),
        observed_at=str(current.get("time") or ""),
        fetched_at=time.time() if fetched_at is None else fetched_at,
    )


def is_humid_weather(
    snapshot: WeatherSnapshot,
    humidity_threshold: int = 75,
    precipitation_probability_threshold: int = 60,
) -> bool:
    if snapshot.humidity_percent is not None and snapshot.humidity_percent >= humidity_threshold:
        return True
    if snapshot.precipitation_probability is not None and snapshot.precipitation_probability >= precipitation_probability_threshold:
        return True
    if (snapshot.precipitation_mm or 0) > 0 or (snapshot.rain_mm or 0) > 0:
        return True
    return False


class WeatherClient:
    def __init__(self, latitude: float, longitude: float, location: str = "盐城", timeout_seconds: float = 8.0) -> None:
        self.latitude = latitude
        self.longitude = longitude
        self.location = location
        self.timeout_seconds = timeout_seconds
        self._cached: WeatherSnapshot | None = None

    def fetch(self) -> WeatherSnapshot:
        query = urllib.parse.urlencode(
            {
                "latitude": self.latitude,
                "longitude": self.longitude,
                "current": "temperature_2m,relative_humidity_2m,precipitation,rain,weather_code",
                "hourly": "precipitation_probability",
                "forecast_days": 1,
                "timezone": "auto",
            }
        )
        with urllib.request.urlopen(f"https://api.open-meteo.com/v1/forecast?{query}", timeout=self.timeout_seconds) as response:
            data = json.load(response)
        snapshot = parse_open_meteo(data, location=self.location, latitude=self.latitude, longitude=self.longitude)
        self._cached = snapshot
        return snapshot

    def latest(self) -> WeatherSnapshot | None:
        return self._cached


class WeatherCareController:
    def __init__(
        self,
        weather_client: WeatherClient,
        send_command: CommandSender,
        *,
        enabled: bool = True,
        humidity_threshold: int = 75,
        precipitation_probability_threshold: int = 60,
        duration_minutes: int = 15,
        interval_seconds: float = 900.0,
        cooldown_seconds: float = 6 * 3600.0,
    ) -> None:
        self.weather_client = weather_client
        self.send_command = send_command
        self.enabled = enabled
        self.humidity_threshold = humidity_threshold
        self.precipitation_probability_threshold = precipitation_probability_threshold
        self.duration_minutes = max(1, duration_minutes)
        self.interval_seconds = max(30.0, interval_seconds)
        self.cooldown_seconds = max(60.0, cooldown_seconds)
        self._stop = threading.Event()
        self._thread: threading.Thread | None = None
        self._running_until = 0.0
        self._last_start_epoch = 0.0
        self._last_error = ""

    def start(self) -> None:
        if not self.enabled:
            return
        if self._thread and self._thread.is_alive():
            return
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, name="weather-care", daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self._thread:
            self._thread.join(timeout=2)

    def _run(self) -> None:
        while not self._stop.wait(self.interval_seconds):
            self.tick()

    def tick(self, now_epoch: float | None = None) -> dict[str, Any]:
        now = time.time() if now_epoch is None else now_epoch
        if self._running_until and now >= self._running_until:
            self._send_hot_air(False, "天气自动除湿停止")
            self._running_until = 0.0
        try:
            snapshot = self.weather_client.fetch()
            self._last_error = ""
        except Exception as exc:
            self._last_error = str(exc)
            return {"ok": False, "error": self._last_error, "running_until": self._running_until}
        if not is_humid_weather(snapshot, self.humidity_threshold, self.precipitation_probability_threshold):
            return {"ok": True, "started": False, "weather": snapshot.public_dict(), "running_until": self._running_until}
        if self._running_until:
            return {"ok": True, "started": False, "reason": "already-running", "weather": snapshot.public_dict(), "running_until": self._running_until}
        if self._last_start_epoch and now - self._last_start_epoch < self.cooldown_seconds:
            return {"ok": True, "started": False, "reason": "cooldown", "weather": snapshot.public_dict(), "running_until": self._running_until}
        result = self._send_hot_air(True, "天气潮湿自动热风除湿")
        if result.get("ok"):
            self._last_start_epoch = now
            self._running_until = now + self.duration_minutes * 60
        return {"ok": bool(result.get("ok")), "started": bool(result.get("ok")), "weather": snapshot.public_dict(), "running_until": self._running_until}

    def status(self) -> dict[str, Any]:
        return {
            "enabled": self.enabled,
            "humidity_threshold": self.humidity_threshold,
            "precipitation_probability_threshold": self.precipitation_probability_threshold,
            "duration_minutes": self.duration_minutes,
            "interval_seconds": self.interval_seconds,
            "cooldown_seconds": self.cooldown_seconds,
            "running_until": self._running_until,
            "last_start_epoch": self._last_start_epoch,
            "last_error": self._last_error,
        }

    def _send_hot_air(self, start: bool, label: str) -> dict[str, Any]:
        action_id = 8 if start else 9
        payload = {
            "id": f"weather-care-{action_id}-{int(time.time() * 1000)}",
            "action": "todo",
            "action_id": action_id,
            "source": "weather-care",
            "care_device": "hot_air",
            "care_phase": "start" if start else "stop",
            "label": label,
        }
        return self.send_command(payload, "weather-care")
