from attiny_uploader.hexfile import HexParseError, parse_intel_hex


SAMPLE_HEX = "\n".join(
    [
        ":020000040000FA",
        ":10000000112233445566778899AABBCCDDEEFF00F8",
        ":00000001FF",
    ]
)


def test_parse_simple_hex():
    image = parse_intel_hex(SAMPLE_HEX)
    assert image.min_address == 0x0000
    assert len(image.data) == 16
    assert image.data[0] == 0x11
    assert image.data[-1] == 0x00


def test_parse_extended_linear_address():
    hex_text = "\n".join(
        [
            ":020000040000FA",
            ":10800000" + ("AA" * 16) + "D0",
            ":00000001FF",
        ]
    )
    image = parse_intel_hex(hex_text)
    assert image.min_address == 0x8000
    assert len(image.data) == 16
    assert image.data[0] == 0xAA


def test_checksum_error():
    bad = SAMPLE_HEX.replace("F8", "00")
    try:
        parse_intel_hex(bad)
    except HexParseError:
        return
    raise AssertionError("expected HexParseError")
