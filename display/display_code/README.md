# Smart Thunderbird Display (ESP32-S3)

In-dash display node for the 1968 Thunderbird restomod. This board runs the TFT UI, LDR auto-dim backlight, and polls a **remote sensor board** over RS-485 Modbus for all measurements (voltage, AFR, coolant/oil/trans temps).

Prior firmware for the original ESP32 lives in [`../display_code_old/`](../display_code_old/).

## Hardware

| Component | Notes |
|-----------|-------|
| MCU | Hosyond ESP32-S3 N16R8 (16 MB flash, 8 MB OPI PSRAM) |
| Display | 320×480 portrait TFT, ILI9488 default (edit `include/tft_setup.h`) |
| Local sensor | LDR only (backlight auto-dim) |
| Bus | RS-485 Modbus RTU to remote sensor board |

### USB ports

The N16R8 board has two USB-C ports:

- **USB** (native) — use this for upload and serial monitor when `ARDUINO_USB_CDC_ON_BOOT=1` is set (default in `platformio.ini`).
- **UART** — CH340 bridge; use if you disable USB CDC flags in `platformio.ini`.

## Pin assignment

Edit [`include/pins.h`](include/pins.h) when breadboarding. Set each `constexpr` from `-1` to your GPIO number.

| Symbol | Purpose | Old ESP32 GPIO |
|--------|---------|----------------|
| `MODBUS_RX` | RS-485 UART RX | 16 |
| `MODBUS_TX` | RS-485 UART TX | 17 |
| `MODBUS_DE_RE` | MAX485 direction | 26 |
| `LDR_ADC` | Light sensor (local) | 4 |
| `BACKLIGHT_PWM` | TFT backlight LEDC | 25 |

TFT SPI and touch pins are set in [`include/tft_setup.h`](include/tft_setup.h) only (TFT_eSPI uses preprocessor macros).

### ESP32-S3 pins to avoid

Do not use strapping or USB pins: **0, 3, 43, 44, 45, 46**.

## Modbus register map

Defined in [`include/modbus_config.h`](include/modbus_config.h). The remote sensor board (Modbus slave) must publish the same map.

| Register | Field | Encoding |
|----------|-------|----------|
| 0 | Coolant temp | tenths °F (185 = 18.5°F) |
| 1 | Oil temp | tenths °F |
| 2 | Trans temp | tenths °F |
| 3 | Battery voltage | hundredths V (1388 = 13.88 V) |
| 4 | AFR | hundredths (1330 = 13.30) |

Slave ID: **1**, baud: **9600**, poll interval: **500 ms**.

## Build and flash

Requires [PlatformIO](https://platformio.org/).

```bash
cd display/display_code

# Main firmware
pio run -e display
pio run -e display -t upload
pio device monitor

# TFT-only smoke test (no Modbus)
pio run -e lvgl_test -t upload
```

## TFT configuration

If the display stays blank or colors are wrong:

1. Confirm the controller IC (default **ILI9488** in `tft_setup.h`).
2. Assign correct SPI pins in `tft_setup.h`.
3. Try uncommenting `#define USE_HSPI_PORT` in `tft_setup.h` on ESP32-S3.
4. Flash `lvgl_test` first to isolate display wiring from Modbus.

## Migration from `display_code_old`

| Change | Detail |
|--------|--------|
| MCU | ESP32 (`esp32dev`) → ESP32-S3 N16R8 |
| Voltage / O2 | Removed local ADC; received via Modbus registers 3–4 |
| Temps | Still Modbus; now one 5-register read |
| LDR / backlight | Still local on display board |
| LVGL buffer | Increased to 10 lines (was 4) for faster refresh on S3 |

## Project layout

```
display_code/
├── platformio.ini
├── src/main.cpp           # Main display firmware
├── src/lvgl_test_main.cpp # TFT bring-up test
└── include/
    ├── pins.h
    ├── modbus_config.h
    ├── tft_setup.h
    └── lv_conf.h
```

## Related

- [smart_tbird README](../../README.md)
- [Archived CAN migration notes](../display_code_old/README_CAN_CONVERSION.md) (historical)
