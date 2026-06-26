"""Intel HEX parsing and validation."""

from __future__ import annotations

from dataclasses import dataclass


class HexParseError(ValueError):
    """Raised when an Intel HEX file is invalid."""


@dataclass(frozen=True)
class HexImage:
    """Contiguous byte image extracted from Intel HEX records."""

    data: bytes
    start_address: int
    min_address: int
    max_address: int


def _parse_hex_nibble_pair(text: str, offset: int) -> int:
    try:
        return int(text[offset : offset + 2], 16)
    except ValueError as exc:
        raise HexParseError(f"invalid hex at offset {offset}: {text!r}") from exc


def parse_intel_hex(text: str) -> HexImage:
    """
    Parse Intel HEX into a contiguous byte image.

    Supports record types 00 (data), 01 (EOF), 02 (extended segment address),
    and 04 (extended linear address). Other record types are ignored.
    """
    if not text.strip():
        raise HexParseError("empty Intel HEX file")

    memory: dict[int, int] = {}
    extended_linear = 0
    extended_segment = 0
    eof_seen = False
    start_address: int | None = None

    for line_no, raw_line in enumerate(text.splitlines(), start=1):
        line = raw_line.strip()
        if not line:
            continue
        if not line.startswith(":"):
            raise HexParseError(f"line {line_no}: missing leading ':'")

        payload = line[1:]
        if len(payload) < 10 or len(payload) % 2 != 0:
            raise HexParseError(f"line {line_no}: malformed record length")

        byte_count = _parse_hex_nibble_pair(payload, 0)
        address = (_parse_hex_nibble_pair(payload, 2) << 8) | _parse_hex_nibble_pair(payload, 4)
        record_type = _parse_hex_nibble_pair(payload, 6)
        data_start = 8
        data_end = data_start + byte_count * 2
        checksum_offset = data_end

        if checksum_offset + 2 != len(payload):
            raise HexParseError(f"line {line_no}: record length mismatch")

        record_bytes = bytes(
            _parse_hex_nibble_pair(payload, data_start + i * 2)
            for i in range(byte_count)
        )
        checksum = _parse_hex_nibble_pair(payload, checksum_offset)
        computed = (
            byte_count
            + ((address >> 8) & 0xFF)
            + (address & 0xFF)
            + record_type
            + sum(record_bytes)
        ) & 0xFF
        if ((computed + checksum) & 0xFF) != 0:
            raise HexParseError(f"line {line_no}: checksum mismatch")

        if record_type == 0x00:
            base = (extended_linear << 16) + (extended_segment << 4) + address
            for index, value in enumerate(record_bytes):
                absolute = base + index
                if absolute in memory and memory[absolute] != value:
                    raise HexParseError(
                        f"line {line_no}: conflicting data at address 0x{absolute:04X}"
                    )
                memory[absolute] = value
                if start_address is None:
                    start_address = absolute
        elif record_type == 0x01:
            eof_seen = True
            break
        elif record_type == 0x02:
            if byte_count != 2:
                raise HexParseError(f"line {line_no}: invalid extended segment record")
            extended_segment = (record_bytes[0] << 8) | record_bytes[1]
            extended_linear = 0
        elif record_type == 0x04:
            if byte_count != 2:
                raise HexParseError(f"line {line_no}: invalid extended linear record")
            extended_linear = (record_bytes[0] << 8) | record_bytes[1]
            extended_segment = 0
        elif record_type == 0x03:
            if byte_count != 4:
                raise HexParseError(f"line {line_no}: invalid start segment record")
            start_address = ((record_bytes[0] << 8) | record_bytes[1]) << 4
            start_address += (record_bytes[2] << 8) | record_bytes[3]
        elif record_type == 0x05:
            if byte_count != 4:
                raise HexParseError(f"line {line_no}: invalid start linear record")
            start_address = (
                (record_bytes[0] << 24)
                | (record_bytes[1] << 16)
                | (record_bytes[2] << 8)
                | record_bytes[3]
            )
        else:
            # Ignore unsupported record types (e.g. 0xFE/0xFD vendor extensions).
            continue

    if not eof_seen:
        raise HexParseError("missing EOF record (type 01)")

    if not memory:
        raise HexParseError("Intel HEX file contains no data records")

    min_address = min(memory)
    max_address = max(memory)
    size = max_address - min_address + 1
    data = bytearray([0xFF] * size)
    for address, value in memory.items():
        data[address - min_address] = value

    if start_address is None:
        start_address = min_address

    return HexImage(
        data=bytes(data),
        start_address=start_address,
        min_address=min_address,
        max_address=max_address,
    )


def load_hex_file(path: str) -> HexImage:
    """Load and parse an Intel HEX file from disk."""
    with open(path, encoding="utf-8", errors="replace") as handle:
        return parse_intel_hex(handle.read())
