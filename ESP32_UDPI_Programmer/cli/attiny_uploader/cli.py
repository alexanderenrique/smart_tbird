"""Command-line interface for attiny-uploader."""

from __future__ import annotations

import argparse
import sys

from attiny_uploader import __version__
from attiny_uploader.constants import DEFAULT_BAUD
from attiny_uploader.uploader import Uploader
from attiny_uploader.ux import ExitCode, Reporter

SUBCOMMANDS = frozenset(
    {
        "upload",
        "read-signature",
        "updi-probe",
        "erase",
        "reset",
        "serial-monitor",
        "test-run",
    }
)
GLOBAL_OPTIONS = frozenset(
    {"--port", "--baud", "--verbose", "--json", "--verify", "--reset"}
)
VALUE_OPTIONS = frozenset({"--port", "--baud"})


def normalize_argv(argv: list[str]) -> list[str]:
    """Reorder argv so global options precede subcommands (argparse limitation)."""
    argv = list(argv)
    if not argv:
        return argv

    has_subcommand = any(arg in SUBCOMMANDS for arg in argv)
    if not has_subcommand:
        hex_index = next(
            (
                index
                for index, arg in enumerate(argv)
                if not arg.startswith("-") and arg.endswith(".hex")
            ),
            None,
        )
        if hex_index is not None:
            return [*argv[:hex_index], "upload", *argv[hex_index:]]
        return argv

    if argv[0] not in SUBCOMMANDS:
        return argv

    command = argv[0]
    global_args: list[str] = []
    other_args: list[str] = []
    index = 1
    while index < len(argv):
        arg = argv[index]
        option = arg.split("=", 1)[0]
        if option in GLOBAL_OPTIONS:
            global_args.append(arg)
            if "=" not in arg and option in VALUE_OPTIONS:
                index += 1
                if index < len(argv):
                    global_args.append(argv[index])
        else:
            other_args.append(arg)
        index += 1

    return global_args + [command] + other_args


def add_common_arguments(parser: argparse.ArgumentParser) -> None:
    parser.add_argument(
        "--port",
        required=True,
        help="Serial port for the ESP32 programmer (e.g. /dev/ttyUSB0, COM3)",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD,
        help=f"USB serial baud rate (default: {DEFAULT_BAUD})",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Enable verbose logging",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Emit machine-readable JSON output",
    )


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="attiny-uploader",
        description=(
            "Upload Intel HEX firmware to an ATtiny target via an ESP32 UPDI programmer."
        ),
    )
    parser.add_argument("--version", action="version", version=f"%(prog)s {__version__}")
    add_common_arguments(parser)
    parser.add_argument(
        "--verify",
        action="store_true",
        help="Verify flash contents after upload",
    )
    parser.add_argument(
        "--reset",
        action="store_true",
        help="Reset target after upload",
    )

    subparsers = parser.add_subparsers(dest="command")
    subparsers.required = False

    upload_parser = subparsers.add_parser(
        "upload",
        help="Upload Intel HEX firmware (default command)",
        description="Upload Intel HEX firmware to the connected target.",
    )
    upload_parser.add_argument("firmware", help="Path to Intel HEX file")

    subparsers.add_parser(
        "read-signature",
        help="Read target device signature bytes",
    )
    subparsers.add_parser(
        "updi-probe",
        help="Test UPDI link (BREAK, init, read SIB) without programming",
    )
    subparsers.add_parser(
        "erase",
        help="Erase target flash",
    )
    subparsers.add_parser(
        "reset",
        help="Pulse target reset line",
    )
    subparsers.add_parser(
        "serial-monitor",
        help="Enable UART bridge and pass through target serial output",
    )
    test_parser = subparsers.add_parser(
        "test-run",
        help="Run a device-side test definition (JSON)",
    )
    test_parser.add_argument("test_file", help="Path to test definition JSON")

    # Legacy usage: attiny-uploader --port /dev/ttyUSB0 firmware.hex
    parser.add_argument(
        "legacy_firmware",
        nargs="?",
        help="Intel HEX firmware file (legacy positional form)",
    )

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(normalize_argv(argv if argv is not None else sys.argv[1:]))

    reporter = Reporter(json_mode=args.json, verbose=args.verbose)

    command = args.command
    if command is None:
        if args.legacy_firmware:
            command = "upload"
        else:
            parser.print_help()
            return int(ExitCode.USAGE)

    uploader = Uploader(
        port=args.port,
        baud=args.baud,
        reporter=reporter,
        verbose=args.verbose,
    )

    if command == "upload":
        hex_path = args.legacy_firmware or args.firmware
        return uploader.upload_hex(
            hex_path,
            verify=args.verify,
            reset=args.reset,
        )
    if command == "read-signature":
        return uploader.read_signature()
    if command == "updi-probe":
        return uploader.updi_probe()
    if command == "erase":
        return uploader.erase()
    if command == "reset":
        return uploader.reset()
    if command == "serial-monitor":
        return uploader.serial_monitor()
    if command == "test-run":
        return uploader.test_run(args.test_file)

    parser.print_help()
    return int(ExitCode.USAGE)


if __name__ == "__main__":
    sys.exit(main())
