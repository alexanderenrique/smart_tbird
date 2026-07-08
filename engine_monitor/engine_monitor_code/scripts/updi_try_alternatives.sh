#!/usr/bin/env bash
# Try alternate UPDI upload paths (prog.py, avrdude serialupdi, pymcuprog).
# Run with scope DISCONNECTED and only Friend PWR powering the target.
#
# Usage:
#   ./scripts/updi_try_alternatives.sh
#   pio run -t updi-try-alternatives

set -u

PORT="${UPDI_PORT:-/dev/cu.usbserial-110}"
DEVICE="${UPDI_DEVICE:-attiny3216}"
PROG="$HOME/.platformio/packages/framework-arduino-megaavr-megatinycore/tools/prog.py"
PYTHON="$HOME/.platformio/penv/bin/python"
HEX="${1:-.pio/build/attiny3216/firmware.hex}"
AVRDUDE="$HOME/.platformio/packages/tool-avrdude/bin/avrdude"
AVRDUDE_CONF="$HOME/.platformio/packages/tool-avrdude/avrdude.conf"

section() { echo ""; echo "========== $* =========="; }

try_cmd() {
  local label="$1"
  shift
  section "$label"
  echo "+ $*"
  echo "---"
  if "$@"; then
    echo ">>> SUCCESS: $label"
    return 0
  fi
  echo ">>> FAILED: $label (exit $?)"
  return 1
}

section "UPDI alternative uploaders"
echo "Port:   $PORT"
echo "Device: $DEVICE"
echo "Hex:    $HEX"
echo ""
echo "Before each attempt:"
echo "  - UPDI Friend 3V/5V switch set to 5V (for 5V ATtiny3216)"
echo "  - Target powered ONLY from Friend PWR (no bench supply)"
echo "  - Scope probes removed"
echo "  - Power-cycle target (unplug PWR 5s)"
echo ""
read -r -p "Press Enter when ready (or Ctrl-C to abort)..."

# 1. Fuse read only — confirms two-way link without flashing
try_cmd "prog.py fuse read @ 115200" \
  "$PYTHON" "$PROG" -t uart -u "$PORT" -b 115200 -d "$DEVICE" --fuses_print -v -v && exit 0

echo "Power-cycle target, then press Enter..."
read -r

try_cmd "prog.py fuse read @ 230400 (-wd 1)" \
  "$PYTHON" "$PROG" -t uart -u "$PORT" -b 230400 -d "$DEVICE" --fuses_print -v -v -wd 1 && exit 0

echo "Power-cycle target, then press Enter..."
read -r

# 2. pymcuprog — install into PlatformIO venv if missing
if ! "$PYTHON" -m pymcuprog --help >/dev/null 2>&1; then
  section "Installing pymcuprog into PlatformIO venv"
  "$PYTHON" -m pip install pymcuprog
fi

try_cmd "pymcuprog erase (unlock locked chip)" \
  "$PYTHON" -m pymcuprog erase --chip-erase-locked-device \
  -t uart -u "$PORT" -d "$DEVICE" -v debug

echo "Power-cycle target, then press Enter..."
read -r

if [[ -f "$HEX" ]]; then
  try_cmd "pymcuprog write @ 115200" \
    "$PYTHON" -m pymcuprog write --erase \
    --tool uart --device "$DEVICE" --uart "$PORT" --clk 115200 \
    --filename "$HEX" -v debug && exit 0

  echo "Power-cycle target, then press Enter..."
  read -r

  try_cmd "pymcuprog write @ 230400" \
    "$PYTHON" -m pymcuprog write --erase \
    --tool uart --device "$DEVICE" --uart "$PORT" --clk 230400 \
    --filename "$HEX" -v debug && exit 0
fi

echo "Power-cycle target, then press Enter..."
read -r

# 3. avrdude serialupdi (different code path — helped some Mac users)
if [[ -x "$AVRDUDE" && -f "$AVRDUDE_CONF" && -f "$HEX" ]]; then
  try_cmd "avrdude serialupdi @ 115200" \
    "$AVRDUDE" -C "$AVRDUDE_CONF" -c serialupdi -p "$DEVICE" \
    -P "$PORT" -b 115200 -v -v -v \
    -U flash:w:"$HEX":i && exit 0

  echo "Power-cycle target, then press Enter..."
  read -r

  try_cmd "avrdude serialupdi @ 230400" \
    "$AVRDUDE" -C "$AVRDUDE_CONF" -c serialupdi -p "$DEVICE" \
    -P "$PORT" -b 230400 -v -v -v \
    -U flash:w:"$HEX":i && exit 0
else
  echo "Skip avrdude: missing tool or $HEX (run pio run first)"
fi

section "All methods failed"
echo "If scope shows TX activity but every tool gets empty receive:"
echo "  1. UPDI Friend voltage switch: must be 5V for 5V chip"
echo "  2. Series resistor on test board UPDI line — bypass with wire (common on dev boards)"
echo "  3. Try Arduino IDE: megaTinyCore + SerialUPDI SLOW + Upload Using Programmer"
echo "  4. Try a different ATtiny3216 chip on the same breadboard"
exit 1
