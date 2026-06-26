from attiny_uploader.cli import build_parser, normalize_argv


def parse(argv: list[str]):
    return build_parser().parse_args(normalize_argv(argv))


def test_legacy_upload_form():
    args = parse(["--port", "/dev/cu.SLAB_USBtoUART", "firmware.hex"])
    assert args.command == "upload"
    assert args.port == "/dev/cu.SLAB_USBtoUART"
    assert args.firmware == "firmware.hex"


def test_upload_subcommand_with_port_after_command():
    args = parse(["upload", "--port", "/dev/cu.SLAB_USBtoUART", "firmware.hex"])
    assert args.command == "upload"
    assert args.port == "/dev/cu.SLAB_USBtoUART"
    assert args.firmware == "firmware.hex"


def test_read_signature_subcommand():
    args = parse(["read-signature", "--port", "/dev/cu.SLAB_USBtoUART"])
    assert args.command == "read-signature"
    assert args.port == "/dev/cu.SLAB_USBtoUART"
