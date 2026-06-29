#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/gpio5_toggle"
pio run -t upload
