# ESP32 ATtiny UPDI Programmer

Toolchain-agnostic ATtiny programming over USB: a Python CLI streams Intel HEX firmware to an ESP32, which executes UPDI flash operations on the target (ATtiny3216 by default).

**Work log:** [denton.works/microelectronics/ESP32_UPDI_Programmer](https://denton.works/microelectronics/ESP32_UPDI_Programmer/)  
**Repository:** [github.com/alexanderenrique/smart_tbird](https://github.com/alexanderenrique/smart_tbird/tree/platformIO/ESP32_UDPI_Programmer)

```
[ Any Build System ] --> firmware.hex --> attiny-uploader (CLI) --USB Serial--> ESP32 --UPDI--> ATtiny3216
```

The ESP32 never parses HEX files. The CLI never implements UPDI. That separation keeps both sides portable and replaceable.

## Hardware wiring (Phase 1)

Default pin map from [`firmware/include/pins.h`](firmware/include/pins.h) (ESP32 DevKit):

| Signal | ESP32 GPIO | Target connection |
|--------|------------|-------------------|
| UPDI RX2 | GPIO 16 | Tie to GPIO 17, then **4.7 kΩ** to ATtiny UPDI |
| UPDI TX2 | GPIO 17 | Tie to GPIO 16, then **4.7 kΩ** to ATtiny UPDI |
| Target reset | GPIO 18 | ATtiny RESET (active low) |
| 3.3V | 3.3V | ATtiny VCC |
| GND | GND | ATtiny GND |

UPDI uses **Serial2** on the secondary UART header (labeled RX2/TX2). Join GPIO 16 and GPIO 17 at a single tie point on the breadboard, then run a **4.7 kΩ** series resistor from that junction to the ATtiny **UPDI** pin. TX (GPIO 17) is configured open-drain in firmware so the tied line idles high.

The DevKit has **two UART headers**:

| Header | GPIO | Used for |
|--------|------|----------|
| Primary (USB chip) | TX=1, RX=3 | PC ↔ ESP32 command protocol (`attiny-uploader`) |
| Secondary (Serial2) | RX=16, TX=17 | UPDI programming (GPIO 16 + 17 tied together) |

Keep programming commands on the USB port. Do **not** wire the ATtiny application UART in Phase 1.

```
ESP32 DevKit                  ATtiny3216
------------                  ----------
GPIO16 (RX2) ----+
                  +---[4.7k]--- UPDI
GPIO17 (TX2) ----+
GPIO18 ---------------------- RESET
3.3V   ---------------------- VCC
GND    ---------------------- GND
```

Power the ATtiny directly from the ESP32 **3.3V** pin. No GPIO is used for power control.

If UPDI is unreliable at the edge of timing, add an optional **470 Ω–1 kΩ** series resistor between GPIO 17 (TX only) and the tie point.

### Phase 2 (not yet implemented)

Once UPDI programming is stable, the application UART bridge will move to separate GPIOs (candidates: GPIO 4/5 or GPIO 21/22) and cross-connect to the ATtiny USART (PA1 = TX, PA2 = RX on megaTinyCore defaults). GPIO 19 will be free for this use. `serial-monitor` is disabled until Phase 2 pins are assigned.

## Quick start

### 1. Flash the ESP32 firmware

Requires [PlatformIO](https://platformio.org/):

```bash
cd firmware
pio run -t upload
pio device monitor
```

On boot you should see:

```
READY ESP32-UPDI v1.0
DEBUG UPDI step 1/3 send double BREAK ... OK
DEBUG UPDI step 2/3 initialize datalink ... OK
DEBUG UPDI STATUSA=0xXX
DEBUG UPDI step 3/3 read SIB ... OK
INFO SIB raw: tinyAVR...
INFO SIB family: ...
INFO waiting for .hex file (use attiny-uploader)
```

If UPDI wiring or target power is wrong, the probe steps will show `FAIL` and you'll see `INFO UPDI not ready - check wiring and target power`.

To re-run the probe without rebooting:

```bash
attiny-uploader updi-probe --port /dev/ttyUSB0
```

While idle, a heartbeat repeats every 30 seconds:

```
INFO alive - waiting for .hex file
```

### 2. Install the host CLI

```bash
cd cli
python3 -m pip install -e .
attiny-uploader --help
```

### 3. Upload firmware

```bash
attiny-uploader --port /dev/ttyUSB0 firmware.hex
```

With verify and reset:

```bash
attiny-uploader --port /dev/ttyUSB0 --verify --reset firmware.hex
```

Explicit subcommand form:

```bash
attiny-uploader upload --port /dev/ttyUSB0 --verify firmware.hex
```

## CLI command reference

Global options (all commands):

| Option | Description |
|--------|-------------|
| `--port PORT` | Serial port (**required**) |
| `--baud N` | USB serial baud (default: `9600`) |
| `--verbose` | Verbose logging |
| `--json` | Machine-readable JSON output |

### Upload (default)

```bash
attiny-uploader --port PORT [--verify] [--reset] firmware.hex
attiny-uploader upload --port PORT [--verify] [--reset] firmware.hex
```

Example output:

```
loaded 4096 bytes (0x8000-0x8FFF)
connected: OK ESP32-UPDI v1.0
uploading: 32/32 (100.0%)
upload complete
verify OK
target reset
```

### read-signature

```bash
attiny-uploader read-signature --port PORT
```

Example:

```
signature: 1E 95 21
```

Expected ATtiny3216 signature: `1E 95 21`.

### updi-probe

Test the UPDI link without programming flash — sends BREAK, initializes UPDI, and reads the System Information Block (SIB):

```bash
attiny-uploader updi-probe --port PORT
```

Use this to verify wiring and target power before uploading firmware. The ESP32 also runs this probe automatically on boot (see serial monitor output).

### erase

```bash
attiny-uploader erase --port PORT
```

### reset

```bash
attiny-uploader reset --port PORT
```

### serial-monitor

**Disabled in Phase 1** (GPIO 16/17 are used for UPDI). Will be re-enabled in Phase 2 on separate GPIOs.

```bash
attiny-uploader serial-monitor --port PORT
```

Press `Ctrl+C` to exit.

### test-run (stub)

```bash
attiny-uploader test-run --port PORT production_test.json
```

Returns `NOT IMPLEMENTED` until device-side tests are added.

## Exit codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 2 | Usage error |
| 3 | File / HEX parse error |
| 4 | Serial port error |
| 5 | Handshake error |
| 6 | Upload error |
| 7 | Verify error |
| 8 | Device operation error |
| 9 | Not implemented |

## Build-system integration

All build systems only need to produce a valid Intel HEX file.

### Arduino IDE

Set **Upload command** (via `platform.local.txt` or board preference override):

```
attiny-uploader --port {serial.port} {build.path}/{build.project_name}.hex
```

### PlatformIO

In `platformio.ini`:

```ini
upload_protocol = custom
upload_command = attiny-uploader --port $UPLOAD_PORT $SOURCE
```

Or invoke manually after build:

```bash
pio run
attiny-uploader --port /dev/ttyUSB0 .pio/build/<env>/firmware.hex
```

### Make / CMake / CI

```bash
make
attiny-uploader --port /dev/ttyUSB0 build/firmware.hex
```

JSON output for CI:

```bash
attiny-uploader --json --port /dev/ttyUSB0 --verify firmware.hex
```

## Host/device protocol

ASCII line commands over USB serial at 9600 baud. Programming data is sent as raw binary chunks (128 bytes) with `OK` acknowledgements.

Human-readable status lines use an `INFO ` prefix. The host CLI ignores these; they are only for serial monitor users.

| Host sends | Device responds |
|------------|-----------------|
| `HELLO` | `OK ESP32-UPDI v1.0` |
| `BEGIN PROGRAM size=N addr=0xADDR` | `OK` |
| `<128-byte binary chunk>` | `OK` (per chunk) |
| `END PROGRAM` | `OK` |
| `VERIFY` | `OK` or `ERROR addr=0xXXXX` |
| `READ_SIGNATURE` | `OK 1E 95 21` |
| `ERASE` | `OK` |
| `RESET` | `OK` |
| `SERIAL ON` / `SERIAL OFF` | `OK` |
| `POWER ON` / `POWER OFF` | `OK` (no-op; target powered from 3.3V) |
| `RUN TEST name` | `OK` or `ERROR NOT IMPLEMENTED` |

Unsolicited status (serial monitor only):

| When | Device prints |
|------|----------------|
| Boot | `INFO waiting for .hex file (use attiny-uploader)` |
| Idle (every 30 s) | `INFO alive - waiting for .hex file` |
| After `BEGIN PROGRAM` | `INFO receiving N bytes...` |
| After `END PROGRAM` | `INFO programming complete - success` |
| After `VERIFY` OK | `INFO verify OK` |

## Troubleshooting

| Symptom | Things to check |
|---------|-----------------|
| Port not found | Correct `/dev/tty*` or `COM*` port; USB cable supports data |
| Handshake timeout | ESP32 firmware flashed; monitor shows `READY ESP32-UPDI v1.0` |
| Upload fails | UPDI wiring and 4.7 kΩ resistor; target powered from 3.3V; GPIO 16 + 17 tied together to UPDI |
| Signature mismatch | Wrong target MCU or bad UPDI connection |
| Verify failure | Power/noise during programming; retry with `--verbose` |

## Project layout

```
ESP32_UDPI_Programmer/
├── README.md           # This file
├── cli/                # attiny-uploader Python package
└── firmware/           # ESP32 PlatformIO project
```

## Development

CLI tests:

```bash
cd cli
python3 -m pip install -e ".[dev]"
pytest
```

Firmware build:

```bash
cd firmware
pio run
```

See [`cli/README.md`](cli/README.md) for CLI development notes.

## License

Use and modify for your project. Confirm Microchip UPDI usage complies with your target deployment requirements.
