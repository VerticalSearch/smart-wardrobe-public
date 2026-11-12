from __future__ import annotations

import json
import threading
import time
from typing import Any, Callable

from .config import GatewayConfig
from .state import GatewayState

try:
    import paho.mqtt.client as mqtt
except Exception:  # pragma: no cover - dependency is installed on the Pi by requirements.
    mqtt = None


LEGACY_COMMAND_TOPIC = "smart_wardrobe/k230-wardrobe-001/cmd"
LEGACY_STATUS_TOPIC = "smart_wardrobe/k230-wardrobe-001/status"


def _is_success_reason(reason_code: Any) -> bool:
    try:
        return int(reason_code) == 0
    except (TypeError, ValueError):
        pass
    value = getattr(reason_code, "value", None)
    if value is not None:
        return value == 0
    return str(reason_code).lower() in {"success", "normal disconnection", "no error"}


class MqttBridge:
    def __init__(
        self,
        config: GatewayConfig,
        state: GatewayState,
        handler: Callable[[dict[str, Any], str], dict[str, Any]],
    ) -> None:
        self.config = config
        self.state = state
        self.handler = handler
        self.client = None
        self._thread: threading.Thread | None = None
        self._stop = threading.Event()

    def _command_topics(self) -> list[str]:
        return list(dict.fromkeys([self.config.mqtt_command_topic, LEGACY_COMMAND_TOPIC]))

    def _status_topics(self) -> list[str]:
        return list(dict.fromkeys([self.config.mqtt_status_topic, LEGACY_STATUS_TOPIC]))

    def start(self) -> None:
        if not self.config.mqtt_enable:
            self.state.mqtt_error = "MQTT disabled"
            return
        if mqtt is None:
            self.state.mqtt_error = "paho-mqtt is not installed"
            return
        if self._thread and self._thread.is_alive():
            return
        self._stop.clear()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        self._stop.set()
        if self.client is not None:
            try:
                self.client.disconnect()
            except Exception:
                pass

    def publish_status(self, status: str, detail: str = "", command_id: str = "", stage: str = "") -> None:
        if not self.client or not self.state.mqtt_connected:
            return
        payload = {
            "device_id": self.config.device_id,
            "source": "raspberry-pi",
            "status": status,
            "detail": detail,
            "command_id": command_id,
            "stage": stage,
            "ts": int(time.time() * 1000),
        }
        try:
            encoded = json.dumps(payload, ensure_ascii=False)
            for topic in self._status_topics():
                self.client.publish(topic, encoded, qos=0)
        except Exception as exc:
            self.state.mqtt_error = str(exc)

    def _run(self) -> None:
        while not self._stop.is_set():
            try:
                self._connect_loop()
            except Exception as exc:
                self.state.mqtt_connected = False
                self.state.mqtt_error = str(exc)
                self.state.add_log("mqtt-error", str(exc))
                time.sleep(5)

    def _connect_loop(self) -> None:
        client_id = f"{self.config.mqtt_client_id}-{int(time.time()) & 0xFFFF}"
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id=client_id)
        if self.config.mqtt_username:
            client.username_pw_set(self.config.mqtt_username, self.config.mqtt_password or None)

        def on_connect(client, userdata, flags, reason_code, properties=None):
            if _is_success_reason(reason_code):
                self.state.mqtt_connected = True
                self.state.mqtt_error = ""
                self.state.add_log("mqtt", f"connected {self.config.mqtt_host}:{self.config.mqtt_port}")
                for topic in self._command_topics():
                    client.subscribe(topic, qos=0)
                self.publish_status("online", "raspberry pi gateway")
            else:
                self.state.mqtt_connected = False
                self.state.mqtt_error = f"connect failed: {reason_code}"

        def on_disconnect(client, userdata, disconnect_flags, reason_code, properties=None):
            self.state.mqtt_connected = False
            if not _is_success_reason(reason_code):
                self.state.mqtt_error = f"disconnect: {reason_code}"

        def on_message(client, userdata, message):
            try:
                payload = json.loads(message.payload.decode("utf-8"))
                if not isinstance(payload, dict):
                    raise ValueError("payload is not an object")
                if self.config.mqtt_command_token and payload.get("token") != self.config.mqtt_command_token:
                    self.publish_status("rejected", "bad_token", str(payload.get("id", "")))
                    return
                self.publish_status("queued", str(payload.get("action", "")), str(payload.get("id", "")), "cloud_rx")
                result = self.handler(payload, source="mqtt")
                self.publish_status(
                    "sent" if result.get("ok") else "error",
                    str(result.get("detail", "")),
                    str(payload.get("id", "")),
                    "uart_tx",
                )
            except Exception as exc:
                self.publish_status("bad_command", str(exc))
                self.state.add_log("mqtt-error", str(exc))

        client.on_connect = on_connect
        client.on_disconnect = on_disconnect
        client.on_message = on_message
        self.client = client
        client.connect(self.config.mqtt_host, self.config.mqtt_port, keepalive=60)
        while not self._stop.is_set():
            client.loop(timeout=1.0)
        client.disconnect()
