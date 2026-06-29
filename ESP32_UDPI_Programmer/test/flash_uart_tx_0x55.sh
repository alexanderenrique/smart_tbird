#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/uart_tx_0x55"
pio run -t upload
