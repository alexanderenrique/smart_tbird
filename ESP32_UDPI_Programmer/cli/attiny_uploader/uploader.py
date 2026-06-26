"""Upload orchestration and device operations."""

from __future__ import annotations

import sys
import time
from typing import Optional

from attiny_uploader.constants import DEFAULT_BAUD
from attiny_uploader.hexfile import HexImage, HexParseError, load_hex_file
from attiny_uploader.protocol import (
    DEFAULT_CHUNK_SIZE,
    DeviceProtocol,
    ProtocolError,
    pad_chunk,
)
from attiny_uploader.transport import SerialTransport
from attiny_uploader.ux import ExitCode, ProgressEvent, Reporter

_ACTION_HINTS: dict[str, str] = {
    "handshake": (
        "Confirm ESP32 firmware is flashed, the correct port is selected, "
        "and no other program (serial monitor) has the port open."
    ),
    "begin program": (
        "Confirm ESP32 firmware is running and the serial port is not in use "
        "by another program."
    ),
    "send chunk": (
        "The host waits for an OK line from the ESP32 after each 128-byte chunk "
        "(this is not ATtiny application serial). The ESP32 programs flash over UPDI "
        "as data arrives, so an unresponsive target or bad UPDI wiring can cause this "
        "timeout. Try --verbose and watch the ESP32 serial monitor for ERROR lines."
    ),
    "end program": (
        "The ESP32 may still be finishing the last UPDI flash write. Check UPDI "
        "wiring and target power, or use --verbose with a serial monitor."
    ),
}


class Uploader:
    """High-level operations against the ESP32 programmer."""

    def __init__(
        self,
        port: str,
        baud: int = DEFAULT_BAUD,
        chunk_size: int = DEFAULT_CHUNK_SIZE,
        retries: int = 2,
        reporter: Optional[Reporter] = None,
        verbose: bool = False,
    ) -> None:
        self.port = port
        self.baud = baud
        self.chunk_size = chunk_size
        self.retries = retries
        self.reporter = reporter or Reporter(verbose=verbose)
        self.verbose = verbose

    def _connect(self) -> tuple[SerialTransport, DeviceProtocol]:
        transport = SerialTransport(self.port, baudrate=self.baud)
        try:
            transport.open()
        except Exception as exc:
            raise RuntimeError(f"failed to open port {self.port}: {exc}") from exc
        protocol = DeviceProtocol(transport, verbose=self.verbose)
        return transport, protocol

    def _with_retries(self, action_name: str, func) -> None:
        last_error: Optional[Exception] = None
        for attempt in range(self.retries + 1):
            try:
                func()
                return
            except (ProtocolError, TimeoutError, RuntimeError) as exc:
                last_error = exc
                self.reporter.debug(
                    f"{action_name} attempt {attempt + 1} failed: {exc}"
                )
                if attempt < self.retries:
                    time.sleep(0.2 * (attempt + 1))
        message = f"{action_name} failed: {last_error}"
        hint = _ACTION_HINTS.get(action_name)
        if hint:
            message = f"{message}. {hint}"
        raise RuntimeError(message) from last_error

    def handshake(self, protocol: DeviceProtocol) -> str:
        response = ""

        def _hello() -> None:
            nonlocal response
            response = protocol.hello()

        self._with_retries("handshake", _hello)
        self.reporter.info(f"connected: {response}")
        return response

    def upload_hex(
        self,
        hex_path: str,
        verify: bool = False,
        reset: bool = False,
    ) -> int:
        try:
            image = load_hex_file(hex_path)
        except (OSError, HexParseError) as exc:
            return self.reporter.fail(str(exc), ExitCode.FILE_ERROR)

        self.reporter.info(
            f"loaded {len(image.data)} bytes "
            f"(0x{image.min_address:04X}-0x{image.max_address:04X})"
        )

        transport: Optional[SerialTransport] = None
        try:
            transport, protocol = self._connect()
            self.handshake(protocol)
            self._stream_image(protocol, image)
            if verify:
                self.reporter.info("verifying flash...")
                try:
                    protocol.verify()
                except ProtocolError as exc:
                    return self.reporter.fail(str(exc), ExitCode.VERIFY_ERROR)
                self.reporter.info("verify OK")
            if reset:
                protocol.reset_target()
                self.reporter.info("target reset")
        except RuntimeError as exc:
            return self.reporter.fail(str(exc), ExitCode.PORT_ERROR)
        except ProtocolError as exc:
            return self.reporter.fail(str(exc), ExitCode.UPLOAD_ERROR)
        finally:
            if transport is not None:
                transport.close()

        return self.reporter.result(
            {
                "operation": "upload",
                "bytes": len(image.data),
                "start_address": f"0x{image.min_address:04X}",
                "verified": verify,
                "reset": reset,
            }
        )

    def _stream_image(self, protocol: DeviceProtocol, image: HexImage) -> None:
        data = image.data
        total_chunks = (len(data) + self.chunk_size - 1) // self.chunk_size

        padded_size = total_chunks * self.chunk_size
        def _begin() -> None:
            protocol.begin_program(padded_size, image.min_address)

        self._with_retries("begin program", _begin)

        for index in range(total_chunks):
            offset = index * self.chunk_size
            chunk = pad_chunk(data[offset : offset + self.chunk_size], self.chunk_size)

            def _send(current_chunk: bytes = chunk) -> None:
                protocol.send_chunk(current_chunk)

            self._with_retries("send chunk", _send)
            self.reporter.progress(
                ProgressEvent(
                    phase="uploading",
                    current=index + 1,
                    total=total_chunks,
                )
            )

        self.reporter.finish_progress()

        def _end() -> None:
            protocol.end_program()

        self._with_retries("end program", _end)
        self.reporter.info("upload complete")

    def read_signature(self) -> int:
        transport: Optional[SerialTransport] = None
        try:
            transport, protocol = self._connect()
            self.handshake(protocol)
            signature = protocol.read_signature()
        except RuntimeError as exc:
            return self.reporter.fail(str(exc), ExitCode.PORT_ERROR)
        except ProtocolError as exc:
            return self.reporter.fail(str(exc), ExitCode.DEVICE_ERROR)
        finally:
            if transport is not None:
                transport.close()

        self.reporter.info(f"signature: {signature.hex_string}")
        return self.reporter.result(
            {
                "operation": "read-signature",
                "signature": signature.hex_string,
            }
        )

    def updi_probe(self) -> int:
        transport: Optional[SerialTransport] = None
        try:
            transport, protocol = self._connect()
            self.handshake(protocol)
            protocol.updi_probe()
        except RuntimeError as exc:
            return self.reporter.fail(str(exc), ExitCode.PORT_ERROR)
        except ProtocolError as exc:
            return self.reporter.fail(str(exc), ExitCode.DEVICE_ERROR)
        finally:
            if transport is not None:
                transport.close()

        self.reporter.info("UPDI probe passed")
        return self.reporter.result({"operation": "updi-probe"})

    def erase(self) -> int:
        transport: Optional[SerialTransport] = None
        try:
            transport, protocol = self._connect()
            self.handshake(protocol)
            protocol.erase()
        except RuntimeError as exc:
            return self.reporter.fail(str(exc), ExitCode.PORT_ERROR)
        except ProtocolError as exc:
            return self.reporter.fail(str(exc), ExitCode.DEVICE_ERROR)
        finally:
            if transport is not None:
                transport.close()

        self.reporter.info("chip erased")
        return self.reporter.result({"operation": "erase"})

    def reset(self) -> int:
        transport: Optional[SerialTransport] = None
        try:
            transport, protocol = self._connect()
            self.handshake(protocol)
            protocol.reset_target()
        except RuntimeError as exc:
            return self.reporter.fail(str(exc), ExitCode.PORT_ERROR)
        except ProtocolError as exc:
            return self.reporter.fail(str(exc), ExitCode.DEVICE_ERROR)
        finally:
            if transport is not None:
                transport.close()

        self.reporter.info("target reset")
        return self.reporter.result({"operation": "reset"})

    def serial_monitor(self) -> int:
        transport: Optional[SerialTransport] = None
        try:
            transport, protocol = self._connect()
            self.handshake(protocol)
            protocol.serial_on()
            self.reporter.info("serial bridge enabled (Ctrl+C to exit)")
            port = transport._require_open()
            while True:
                if port.in_waiting:
                    sys.stdout.write(
                        port.read(port.in_waiting).decode("utf-8", errors="replace")
                    )
                    sys.stdout.flush()
                if sys.stdin in select_poll():
                    line = sys.stdin.readline()
                    if not line:
                        break
                    port.write(line.encode("utf-8"))
        except KeyboardInterrupt:
            self.reporter.info("serial monitor stopped")
        except RuntimeError as exc:
            return self.reporter.fail(str(exc), ExitCode.PORT_ERROR)
        except ProtocolError as exc:
            return self.reporter.fail(str(exc), ExitCode.DEVICE_ERROR)
        finally:
            if transport is not None:
                try:
                    protocol.serial_off()
                except Exception:
                    pass
                transport.close()

        return self.reporter.result({"operation": "serial-monitor"})

    def test_run(self, test_file: str) -> int:
        import json

        try:
            with open(test_file, encoding="utf-8") as handle:
                payload = json.load(handle)
        except OSError as exc:
            return self.reporter.fail(str(exc), ExitCode.FILE_ERROR)

        test_name = payload.get("name")
        if not test_name:
            return self.reporter.fail(
                "test file must include a 'name' field", ExitCode.FILE_ERROR
            )

        transport: Optional[SerialTransport] = None
        try:
            transport, protocol = self._connect()
            self.handshake(protocol)
            protocol.run_test(str(test_name))
        except RuntimeError as exc:
            return self.reporter.fail(str(exc), ExitCode.PORT_ERROR)
        except ProtocolError as exc:
            message = str(exc)
            if "NOT IMPLEMENTED" in message.upper():
                return self.reporter.fail(message, ExitCode.NOT_IMPLEMENTED)
            return self.reporter.fail(message, ExitCode.DEVICE_ERROR)
        finally:
            if transport is not None:
                transport.close()

        self.reporter.info(f"test passed: {test_name}")
        return self.reporter.result(
            {"operation": "test-run", "test": test_name, "file": test_file}
        )


def select_poll():
    """Return stdin if readable; cross-platform best effort."""
    import sys

    try:
        import select

        ready, _, _ = select.select([sys.stdin], [], [], 0.05)
        return ready
    except Exception:
        return []
