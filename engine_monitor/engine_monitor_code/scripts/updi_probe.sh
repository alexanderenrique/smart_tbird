#!/usr/bin/env bash
# Verbose SerialUPDI / UPDI link diagnostics for ATtiny3216.
# Usage:
#   ./scripts/updi_probe.sh
#   UPDI_PORT=/dev/cu.usbserial-110 ./scripts/updi_probe.sh
#   pio run -t updi-probe

set -u

PORT="${UPDI_PORT:-/dev/cu.usbserial-110}"
DEVICE="${UPDI_DEVICE:-attiny3216}"
PROG="${PROG_PY:-$HOME/.platformio/packages/framework-arduino-megaavr-megatinycore/tools/prog.py}"
PYTHON="${PYTHON:-$HOME/.platformio/penv/bin/python}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

section() { echo ""; echo "========== $* =========="; }
ok()   { echo -e "${GREEN}OK${NC}: $*"; }
warn() { echo -e "${YELLOW}WARN${NC}: $*"; }
fail() { echo -e "${RED}FAIL${NC}: $*"; }

section "UPDI probe"
echo "Port:   $PORT"
echo "Device: $DEVICE"
echo "Prog:   $PROG"
echo "Python: $PYTHON"

section "1. Serial ports (pio device list)"
if command -v pio >/dev/null 2>&1; then
  pio device list || true
else
  warn "pio not in PATH; skipping device list"
fi

section "2. Port file check"
if [[ -e "$PORT" ]]; then
  ok "Port exists: $PORT"
  ls -l "$PORT"
else
  fail "Port not found: $PORT"
  echo "Set UPDI_PORT to the port from 'pio device list' and retry."
  exit 1
fi

section "3. Port open check (stty)"
if stty -f "$PORT" 57600 cs8 -cstopb -parenb 2>/dev/null; then
  ok "Can configure $PORT at 57600 8N1"
else
  warn "stty could not configure port (may be busy or driver quirk)"
fi

section "4. prog.py sanity"
if [[ ! -x "$PYTHON" && ! -f "$PYTHON" ]]; then
  fail "Python not found: $PYTHON"
  exit 1
fi
if [[ ! -f "$PROG" ]]; then
  fail "prog.py not found: $PROG"
  exit 1
fi
ok "prog.py and python found"

run_probe() {
  local label="$1"
  shift
  section "$label"
  echo "Command: $PYTHON $PROG $*"
  echo "---"
  if "$PYTHON" "$PROG" "$@"; then
    ok "$label succeeded"
    return 0
  fi
  fail "$label failed (exit $?)"
  return 1
}

# Try several baud rates and Mac CH340 workarounds.
BAUDS=(57600 115200 230400)
WRITE_DELAYS=(0 1)

for wd in "${WRITE_DELAYS[@]}"; do
  for baud in "${BAUDS[@]}"; do
    extra=()
    if [[ "$wd" != "0" ]]; then
      extra+=(-wd "$wd")
    fi
    if run_probe "Fuse read @ ${baud} baud, writedelay=${wd}ms" \
      -t uart -u "$PORT" -b "$baud" -d "$DEVICE" \
      --fuses_print -v -v ${extra[@]+"${extra[@]}"}; then
      section "Result"
      ok "UPDI link works at ${baud} baud (writedelay=${wd}ms)"
      echo "Update platformio.ini if needed:"
      echo "  upload_speed = $baud"
      if [[ "$wd" != "0" ]]; then
        echo "  Add -wd $wd to upload_flags"
      fi
      exit 0
    fi
    echo ""
    echo "Look for lines like:"
    echo "  'Can't read CS register' + 'receive : []'  -> no UPDI response from target"
    echo "  'Device is locked'                         -> try power-cycle, then retry"
    echo "  'Device ID mismatch'                       -> wrong part or bad connection"
    echo ""
  done
done

section "Result"
fail "UPDI link failed at all tested baud rates (57600, 115200, 230400)"
echo ""
echo "Hardware checklist:"
echo "  - UPDI Friend PWR -> ATtiny VCC (5V)"
echo "  - UPDI Friend GND -> ATtiny GND"
echo "  - UPDI Friend UPDI -> PA0 (physical pin 16, not pin 18/PA2)"
echo "  - Power-cycle the ATtiny after any failed attempt"
echo "  - No scope/multimeter on the UPDI line during programming"
echo "  - Common ground between USB adapter and any bench supply"
exit 1
