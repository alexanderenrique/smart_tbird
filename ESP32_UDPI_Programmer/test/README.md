# Hardware test firmware

Minimal ESP32 sketches for scope and wiring checks. Each subfolder is a standalone PlatformIO project — flash it instead of the main `firmware/` build while debugging.

**USB serial monitor:** All test sketches use **115200 baud** on the primary UART (GPIO 1/3). `test/platformio_common.ini` sets `monitor_speed` and `USB_SERIAL_BAUD` together — keep them in sync if you change either. Open the monitor from the project you flashed:

```bash
cd test/gpio5_toggle   # or whichever test you uploaded
pio device monitor -b 115200
```

Or: `pio run -t monitor` (reads `monitor_speed` from that project's `platformio.ini`).

## uart_tx_0x55

Streams **`0x55` continuously** on **Serial2 TX (GPIO 17)** at **115200 baud** (production UPDI settings: **8E2 + open-drain TX**).

`0x55` is `01010101` — alternating data bits inside each UART frame. USB serial prints loopback stats once per second (`TX` vs `RX_echo` on GPIO 16).

### Flash

```bash
cd test/uart_tx_0x55
pio run -t upload
```

Or from the repo root:

```bash
./test/flash_uart_tx_0x55.sh
```

### Scope hookup

| Probe | ESP32 pin |
|-------|-----------|
| Signal | GPIO 17 (TX2 on the secondary UART header) |
| Ground | GND |

Default build uses **8E2 + open-drain TX** (same as production UPDI). For plain push-pull 8N1, remove `-DMATCH_UPDI_PHY=1` from `uart_tx_0x55/platformio.ini` and reflash.

### What you should see

- **Scope on GPIO 17:** continuous UART frames; data bits alternate ~8.7 µs high / ~8.7 µs low at 115200.
- **USB monitor:** `loopback TX=… RX_echo=…` — if GPIO 16 and 17 are tied, `RX_echo` should track `TX`.
- Line idles **high** between bytes.
- If the line is stuck low or flat, check wiring, ground, and that you are on GPIO 17 (not the USB UART on GPIO 1).

## tx2_toggle_1hz

Drives **GPIO 17 (TX2)** as a plain GPIO output: **500 ms high, 500 ms low** — a **1 Hz** square wave. No UART traffic; the sketch calls `Serial2.end()` and `gpio_reset_pin()` first because GPIO 17 is UART2 TX and `digitalWrite` alone cannot pull it low reliably.

### Flash

```bash
cd test/tx2_toggle_1hz
pio run -t upload
```

Or from the repo root:

```bash
./test/flash_tx2_toggle_1hz.sh
```

### Scope hookup

| Probe | ESP32 pin |
|-------|-----------|
| Signal | GPIO 17 (TX2 on the secondary UART header) |
| Ground | GND |

### What you should see

- **1 Hz** square wave: ~500 ms high, ~500 ms low.
- Probe **GPIO 17 on the ESP32 header**, not the far side of the 4.7 kΩ UPDI resistor — a powered target can hold that line high.
- If GPIO 16 and 17 are tied on the breadboard, this sketch floats GPIO 16 as an input so it does not fight TX.
- Change `HALF_PERIOD_MS` in `platformio.ini` to adjust frequency (period = 2 × `HALF_PERIOD_MS`).

## gpio5_toggle

Square wave on a **plain GPIO** (default **GPIO 5**): **500 ms high, 500 ms low** — **1 Hz** by default. Change `HALF_PERIOD_MS` in `platformio.ini` (that build flag overrides any value in `main.cpp`).

### Flash

```bash
cd test/gpio5_toggle
pio run -t upload
```

Or from the repo root:

```bash
./test/flash_gpio5_toggle.sh
```

### Scope hookup

| Probe | ESP32 pin |
|-------|-----------|
| Signal | GPIO 5 |
| Ground | GND |

### What you should see

- **1 Hz** by default (~500 ms high, ~500 ms low). Period = 2 × `HALF_PERIOD_MS` in `platformio.ini`.
