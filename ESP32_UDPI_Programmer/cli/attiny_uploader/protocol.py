"""Device protocol framing and response parsing."""

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import Optional

from attiny_uploader.constants import DEFAULT_BAUD
from attiny_uploader.transport import SerialTransport

DEVICE_ID = "ESP32-UPDI v1.0"
DEFAULT_CHUNK_SIZE = 128


class ProtocolError(RuntimeError):
    """Raised when the device returns an unexpected response."""


@dataclass(frozen=True)
class SignatureResponse:
    signature: bytes

    @property
    def hex_string(self) -> str:
        return " ".join(f"{byte:02X}" for byte in self.signature)


class DeviceProtocol:
    """ASCII control protocol with raw binary programming stream."""

    def __init__(self, transport: SerialTransport, verbose: bool = False) -> None:
        self.transport = transport
        self.verbose = verbose

    def _log(self, message: str) -> None:
        if self.verbose:
            print(f"[protocol] {message}")

    def _expect_ok(self, response: str) -> None:
        if response != "OK":
            raise ProtocolError(response)

    def _read_response(self, waiting_for: str) -> str:
        return self.transport.read_line(waiting_for=waiting_for)

    def hello(self) -> str:
        self._log("HELLO")
        self.transport.write_line("HELLO")
        response = self._read_response("HELLO acknowledgement from ESP32")
        expected_prefix = f"OK {DEVICE_ID}"
        if not response.startswith(expected_prefix):
            raise ProtocolError(f"handshake failed: {response}")
        return response

    def begin_program(self, size: int, start_address: int) -> None:
        command = f"BEGIN PROGRAM size={size} addr=0x{start_address:X}"
        self._log(command)
        self.transport.write_line(command)
        self._expect_ok(self._read_response("OK after BEGIN PROGRAM from ESP32"))

    def send_chunk(self, chunk: bytes) -> None:
        if not chunk:
            return
        self._log(f"chunk {len(chunk)} bytes")
        self.transport.write(chunk)
        self._expect_ok(
            self._read_response(
                "OK after binary chunk from ESP32 (not ATtiny application serial)"
            )
        )

    def end_program(self) -> None:
        self._log("END PROGRAM")
        self.transport.write_line("END PROGRAM")
        self._expect_ok(self._read_response("OK after END PROGRAM from ESP32"))

    def verify(self) -> None:
        self._log("VERIFY")
        self.transport.write_line("VERIFY")
        response = self._read_response("VERIFY result from ESP32")
        if response != "OK":
            match = re.match(r"ERROR addr=0x([0-9A-Fa-f]+)", response)
            if match:
                raise ProtocolError(f"verify failed at address 0x{match.group(1)}")
            raise ProtocolError(response)

    def read_signature(self) -> SignatureResponse:
        self._log("READ_SIGNATURE")
        self.transport.write_line("READ_SIGNATURE")
        response = self._read_response("signature response from ESP32")
        if not response.startswith("OK "):
            raise ProtocolError(response)
        parts = response[3:].strip().split()
        try:
            signature = bytes(int(part, 16) for part in parts)
        except ValueError as exc:
            raise ProtocolError(f"invalid signature response: {response}") from exc
        return SignatureResponse(signature=signature)

    def updi_probe(self) -> None:
        self._log("UPDI_PROBE")
        self.transport.write_line("UPDI_PROBE")
        self._expect_ok(self._read_response("OK after UPDI_PROBE from ESP32"))

    def erase(self) -> None:
        self._log("ERASE")
        self.transport.write_line("ERASE")
        self._expect_ok(self._read_response("OK after ERASE from ESP32"))

    def reset_target(self) -> None:
        self._log("RESET")
        self.transport.write_line("RESET")
        self._expect_ok(self._read_response("OK after RESET from ESP32"))

    def serial_on(self) -> None:
        self._log("SERIAL ON")
        self.transport.write_line("SERIAL ON")
        self._expect_ok(self._read_response("OK after SERIAL ON from ESP32"))

    def serial_off(self) -> None:
        self._log("SERIAL OFF")
        self.transport.write_line("SERIAL OFF")
        self._expect_ok(self._read_response("OK after SERIAL OFF from ESP32"))

    def power_on(self) -> None:
        self._log("POWER ON")
        self.transport.write_line("POWER ON")
        self._expect_ok(self._read_response("OK after POWER ON from ESP32"))

    def power_off(self) -> None:
        self._log("POWER OFF")
        self.transport.write_line("POWER OFF")
        self._expect_ok(self._read_response("OK after POWER OFF from ESP32"))

    def run_test(self, test_name: str) -> None:
        command = f"RUN TEST {test_name}"
        self._log(command)
        self.transport.write_line(command)
        response = self._read_response("test result from ESP32")
        if response.startswith("ERROR"):
            raise ProtocolError(response)
        if not response.startswith("OK"):
            raise ProtocolError(response)


def pad_chunk(chunk: bytes, chunk_size: int = DEFAULT_CHUNK_SIZE) -> bytes:
    """Pad a flash chunk to page size with erased-byte value 0xFF."""
    if len(chunk) >= chunk_size:
        return chunk[:chunk_size]
    return chunk + bytes([0xFF]) * (chunk_size - len(chunk))
