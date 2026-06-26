"""User-facing output helpers and exit codes."""

from __future__ import annotations

import json
import sys
from dataclasses import asdict, dataclass
from enum import IntEnum
from typing import Any, Optional


class ExitCode(IntEnum):
    OK = 0
    USAGE = 2
    FILE_ERROR = 3
    PORT_ERROR = 4
    HANDSHAKE_ERROR = 5
    UPLOAD_ERROR = 6
    VERIFY_ERROR = 7
    DEVICE_ERROR = 8
    NOT_IMPLEMENTED = 9


@dataclass
class ProgressEvent:
    phase: str
    current: int
    total: int
    message: str = ""


class Reporter:
    """Human-readable or JSON output."""

    def __init__(self, json_mode: bool = False, verbose: bool = False) -> None:
        self.json_mode = json_mode
        self.verbose = verbose
        self._events: list[dict[str, Any]] = []

    def info(self, message: str) -> None:
        if self.json_mode:
            self._events.append({"level": "info", "message": message})
        else:
            print(message)

    def debug(self, message: str) -> None:
        if not self.verbose:
            return
        if self.json_mode:
            self._events.append({"level": "debug", "message": message})
        else:
            print(message, file=sys.stderr)

    def error(self, message: str) -> None:
        if self.json_mode:
            self._events.append({"level": "error", "message": message})
        else:
            print(f"error: {message}", file=sys.stderr)

    def progress(self, event: ProgressEvent) -> None:
        if self.json_mode:
            self._events.append({"level": "progress", **asdict(event)})
            return
        if event.total > 0:
            percent = (event.current / event.total) * 100.0
            print(
                f"\r{event.phase}: {event.current}/{event.total} ({percent:5.1f}%)",
                end="",
                flush=True,
            )
        else:
            print(event.message)

    def finish_progress(self) -> None:
        if not self.json_mode:
            print()

    def result(self, payload: dict[str, Any], exit_code: ExitCode = ExitCode.OK) -> int:
        if self.json_mode:
            output = {"status": "ok" if exit_code == ExitCode.OK else "error", **payload}
            if self._events:
                output["events"] = self._events
            print(json.dumps(output, indent=2))
        return int(exit_code)

    def fail(self, message: str, exit_code: ExitCode) -> int:
        self.error(message)
        if self.json_mode:
            print(
                json.dumps(
                    {
                        "status": "error",
                        "message": message,
                        "exit_code": int(exit_code),
                        "events": self._events,
                    },
                    indent=2,
                )
            )
        return int(exit_code)
