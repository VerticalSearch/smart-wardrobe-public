from __future__ import annotations

import glob
import os
import threading
import time
from pathlib import Path
from typing import Any, Callable

from .config import GatewayConfig
from .protocol import FRAME_HEADER, checksum
from .state import GatewayState

try:
    import serial
except Exception:  # pragma: no cover - only hit when dependency is absent on target.
    serial = None


class SerialLink:
    def __init__(self, config: GatewayConfig, state: GatewayState) -> None:
        self.config = config
        self.state = state
        self._serial = None
        self._lock = threading.Lock()
        self._reader_thread: threading.Thread | None = None
        self._stop = threading.Event()
        self._rx_handler: Callable[[dict[str, Any]], None] | None = None

    def set_rx_handler(self, handler: Callable[[dict[str, Any]], None] | None) -> None:
        self._rx_handler = handler

    def resolve_port(self) -> str | None:
        requested = self.config.serial_port
        if requested and requested != "auto":
            return requested

        candidates: list[str] = []
        candidates.extend(sorted(glob.glob("/dev/serial/by-id/*")))
        candidates.extend(sorted(glob.glob("/dev/ttyUSB*")))
        candidates.extend(sorted(glob.glob("/dev/ttyACM*")))
        for item in candidates:
            if Path(item).exists():
                return item
        return None

    def open(self) -> bool:
        if self.config.serial_dry_run:
            self.state.serial_connected = False
            self.state.serial_port = "dry-run"
            self.state.serial_error = ""
            self.state.add_log("serial", "serial dry-run enabled")
            return False

        if serial is None:
            self.state.serial_connected = False
            self.state.serial_error = "pyserial is not installed"
            return False

        port = self.resolve_port()
        if not port:
            self.state.serial_connected = False
            self.state.serial_port = ""
            self.state.serial_error = "CH340 serial port not found"
            return False

        try:
            self._serial = serial.Serial(port, self.config.serial_baud, timeout=0.05, write_timeout=1)
            self.state.serial_connected = True
            self.state.serial_port = port
            self.state.serial_error = ""
            self.state.add_log("serial", f"opened {port} @ {self.config.serial_baud}")
            self._start_reader()
            return True
        except Exception as exc:
            self._serial = None
            self.state.serial_connected = False
            self.state.serial_port = port
            self.state.serial_error = str(exc)
            self.state.add_log("serial-error", f"open failed: {exc}")
            return False

    def ensure_open(self) -> bool:
        with self._lock:
            if self._serial is not None and getattr(self._serial, "is_open", False):
                return True
        return self.open()

    def close(self) -> None:
        self._stop.set()
        with self._lock:
            if self._serial is not None:
                try:
                    self._serial.close()
                except Exception:
                    pass
            self._serial = None
        if self._reader_thread and self._reader_thread.is_alive():
            self._reader_thread.join(timeout=0.3)
        self._reader_thread = None
        self.state.serial_connected = False

    def write_line(self, line: str) -> bool:
        data = (line.rstrip("\r\n") + "\n").encode("utf-8")
        return self.write_bytes(data, display=line)

    def write_bytes(self, data: bytes, display: str | None = None) -> bool:
        if self.config.serial_dry_run:
            self.state.add_log("tx-dry", display or data.hex(" "))
            return True
        if not self.ensure_open():
            self.state.add_log("tx-error", "serial not connected", {"data": display or data.hex(" ")})
            return False

        with self._lock:
            try:
                if self._serial is None:
                    raise RuntimeError("serial not open")
                self._serial.write(data)
                self._serial.flush()
                self.state.serial_connected = True
                self.state.serial_error = ""
                self.state.add_log("tx", display or data.hex(" "))
                return True
            except Exception as exc:
                self.state.serial_connected = False
                self.state.serial_error = str(exc)
                self.state.add_log("tx-error", str(exc), {"data": display or data.hex(" ")})
                try:
                    if self._serial is not None:
                        self._serial.close()
                except Exception:
                    pass
                self._serial = None
                return False

    def _start_reader(self) -> None:
        if self._reader_thread and self._reader_thread.is_alive():
            return
        self._stop.clear()
        self._reader_thread = threading.Thread(target=self._read_loop, daemon=True)
        self._reader_thread.start()

    def _read_loop(self) -> None:
        buffer = bytearray()
        while not self._stop.is_set():
            ser = self._serial
            if ser is None:
                time.sleep(0.2)
                continue
            try:
                chunk = ser.read(128)
            except Exception as exc:
                self.state.serial_connected = False
                self.state.serial_error = str(exc)
                self.state.add_log("rx-error", str(exc))
                time.sleep(0.5)
                continue

            if not chunk:
                time.sleep(0.02)
                continue

            buffer.extend(chunk)
            buffer = self._parse_buffer(buffer)

    def _emit(self, event: dict[str, Any]) -> None:
        handler = self._rx_handler
        if handler is None:
            return
        try:
            handler(event)
        except Exception as exc:
            self.state.add_log("rx-handler-error", str(exc), {"event": event})

    def _parse_buffer(self, buffer: bytearray) -> bytearray:
        while buffer:
            first = buffer[0]

            if first == ord("@"):
                newline = buffer.find(b"\n")
                if newline < 0:
                    if len(buffer) > 512:
                        self.state.add_log("rx-drop", "ascii line too long")
                        return bytearray()
                    return buffer
                line_bytes = bytes(buffer[:newline]).rstrip(b"\r")
                del buffer[: newline + 1]
                text = line_bytes.decode("utf-8", errors="replace")
                if text:
                    self.state.add_log("rx", text)
                    self._emit({"type": "line", "line": text})
                continue

            if first == FRAME_HEADER:
                if len(buffer) < 5:
                    return buffer
                frame = bytes(buffer[:5])
                if checksum(frame[:-1]) != frame[-1]:
                    self.state.add_log("rx-bad-frame", frame.hex(" "))
                    del buffer[0]
                    continue
                del buffer[:5]
                self.state.add_log("rx-frame", frame.hex(" "))
                self._emit({"type": "frame", "cmd": frame[1], "p1": frame[2], "p2": frame[3], "raw": frame.hex(" ")})
                continue

            newline = buffer.find(b"\n")
            if newline >= 0:
                raw = bytes(buffer[:newline]).rstrip(b"\r")
                del buffer[: newline + 1]
                if raw:
                    self.state.add_log("rx-bytes", raw.hex(" "))
                continue

            if len(buffer) > 512:
                self.state.add_log("rx-bytes", bytes(buffer).hex(" "))
                return bytearray()

            return buffer

        return buffer


def current_user_needs_dialout() -> bool:
    try:
        return os.name == "posix" and "dialout" not in os.popen("groups").read().split()
    except Exception:
        return False
