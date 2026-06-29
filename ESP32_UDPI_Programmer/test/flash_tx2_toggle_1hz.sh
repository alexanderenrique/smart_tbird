#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/tx2_toggle_1hz"
pio run -t upload
