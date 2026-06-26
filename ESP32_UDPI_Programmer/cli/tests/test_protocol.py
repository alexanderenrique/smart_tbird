import pytest

from attiny_uploader.protocol import DEFAULT_BAUD, DeviceProtocol, ProtocolError
from attiny_uploader.transport import SerialTransport


class MockSerial:
    def __init__(self, responses: list[bytes]):
        self._responses = responses
        self._response_index = 0
        self.written: list[bytes] = []
        self.timeout = 2.0
        self.is_open = True

    def write(self, data: bytes) -> int:
        self.written.append(data)
        return len(data)

    def flush(self) -> None:
        return None

    def readline(self) -> bytes:
        if self._response_index >= len(self._responses):
            return b""
        line = self._responses[self._response_index]
        self._response_index += 1
        return line if line.endswith(b"\n") else line + b"\n"

    def read(self, size: int) -> bytes:
        return b""


def test_handshake_and_begin_program():
    mock = MockSerial(
        [
            b"OK ESP32-UPDI v1.0",
            b"OK",
        ]
    )
    transport = SerialTransport.__new__(SerialTransport)
    transport._serial = mock  # type: ignore[attr-defined]
    transport.port = "mock"
    transport.baudrate = DEFAULT_BAUD
    transport.timeout = 2.0
    transport.write_timeout = 2.0

    protocol = DeviceProtocol(transport)
    response = protocol.hello()
    assert response.startswith("OK ESP32-UPDI")
    protocol.begin_program(128, 0x8000)
    assert b"HELLO\n" in mock.written[0]
    assert b"BEGIN PROGRAM size=128 addr=0x8000\n" in mock.written[1]


def test_send_chunk_expects_ok():
    mock = MockSerial([b"OK"])
    transport = SerialTransport.__new__(SerialTransport)
    transport._serial = mock  # type: ignore[attr-defined]
    transport.port = "mock"
    transport.baudrate = DEFAULT_BAUD
    transport.timeout = 2.0
    transport.write_timeout = 2.0

    protocol = DeviceProtocol(transport)
    protocol.send_chunk(b"\xFF" * 128)
    assert mock.written[-1] == b"\xFF" * 128


def test_verify_error_parsing():
    mock = MockSerial([b"ERROR addr=0x1204"])
    transport = SerialTransport.__new__(SerialTransport)
    transport._serial = mock  # type: ignore[attr-defined]
    transport.port = "mock"
    transport.baudrate = DEFAULT_BAUD
    transport.timeout = 2.0
    transport.write_timeout = 2.0

    protocol = DeviceProtocol(transport)
    with pytest.raises(ProtocolError, match="0x1204"):
        protocol.verify()


def test_read_line_skips_info_messages():
    mock = MockSerial(
        [
            b"INFO alive - waiting for .hex file",
            b"OK ESP32-UPDI v1.0",
        ]
    )
    transport = SerialTransport.__new__(SerialTransport)
    transport._serial = mock  # type: ignore[attr-defined]
    transport.port = "mock"
    transport.baudrate = DEFAULT_BAUD
    transport.timeout = 2.0
    transport.write_timeout = 2.0

    assert transport.read_line() == "OK ESP32-UPDI v1.0"


def test_read_line_skips_debug_messages():
    mock = MockSerial(
        [
            b"DEBUG UPDI step 1/3 send double BREAK ... OK",
            b"INFO SIB family: tiny3216",
            b"OK",
        ]
    )
    transport = SerialTransport.__new__(SerialTransport)
    transport._serial = mock  # type: ignore[attr-defined]
    transport.port = "mock"
    transport.baudrate = DEFAULT_BAUD
    transport.timeout = 2.0
    transport.write_timeout = 2.0

    assert transport.read_line() == "OK"


def test_read_line_timeout_includes_context():
    mock = MockSerial([])
    transport = SerialTransport.__new__(SerialTransport)
    transport._serial = mock  # type: ignore[attr-defined]
    transport.port = "mock"
    transport.baudrate = DEFAULT_BAUD
    transport.timeout = 2.0
    transport.write_timeout = 2.0

    with pytest.raises(TimeoutError, match="OK after binary chunk from ESP32"):
        transport.read_line(waiting_for="OK after binary chunk from ESP32")
