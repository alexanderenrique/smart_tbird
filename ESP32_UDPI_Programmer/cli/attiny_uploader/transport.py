"""Serial transport wrapper."""

from __future__ import annotations

import time
from typing import Optional

import serial

from attiny_uploader.constants import DEFAULT_BAUD


class SerialTransport:
    """Thin pyserial wrapper with consistent timeouts."""

    def __init__(
        self,
        port: str,
        baudrate: int = DEFAULT_BAUD,
        timeout: float = 2.0,
        write_timeout: float = 2.0,
    ) -> None:
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.write_timeout = write_timeout
        self._serial: Optional[serial.Serial] = None

    def open(self) -> None:
        self._serial = serial.Serial(
            port=self.port,
            baudrate=self.baudrate,
            timeout=self.timeout,
            write_timeout=self.write_timeout,
        )
        # Allow ESP32 USB CDC to settle after port open.
        time.sleep(0.2)
        assert self._serial is not None
        self._serial.reset_input_buffer()
        self._serial.reset_output_buffer()

    def close(self) -> None:
        if self._serial and self._serial.is_open:
            self._serial.close()
        self._serial = None

    def __enter__(self) -> "SerialTransport":
        self.open()
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    @property
    def is_open(self) -> bool:
        return self._serial is not None and self._serial.is_open

    def _require_open(self) -> serial.Serial:
        if not self.is_open or self._serial is None:
            raise RuntimeError("serial port is not open")
        return self._serial

    def write(self, data: bytes) -> None:
        port = self._require_open()
        written = port.write(data)
        port.flush()
        if written != len(data):
            raise RuntimeError("short write on serial port")

    def write_line(self, line: str) -> None:
        self.write(line.encode("ascii") + b"\n")

    def read_line(
        self,
        timeout: Optional[float] = None,
        waiting_for: str = "response line from ESP32",
    ) -> str:
        port = self._require_open()
        previous_timeout = port.timeout
        effective_timeout = self.timeout if timeout is None else timeout
        if timeout is not None:
            port.timeout = timeout
        try:
            while True:
                raw = port.readline()
                if not raw:
                    raise TimeoutError(
                        f"no {waiting_for} within {effective_timeout:g}s"
                    )
                line = raw.decode("ascii", errors="replace").strip()
                if line.startswith("INFO ") or line.startswith("DEBUG "):
                    continue
                return line
        finally:
            port.timeout = previous_timeout

    def read_exact(self, size: int, timeout: Optional[float] = None) -> bytes:
        port = self._require_open()
        previous_timeout = port.timeout
        if timeout is not None:
            port.timeout = timeout
        try:
            data = port.read(size)
        finally:
            port.timeout = previous_timeout

        if len(data) != size:
            raise TimeoutError(f"timed out waiting for {size} bytes")
        return data
