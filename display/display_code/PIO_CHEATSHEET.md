# PlatformIO cheat sheet

Run all commands from `display/display_code/`:

```bash
cd display/display_code
```

pio run -e display
pio run -e display -t upload

## Main firmware (`display`)

Use `display_n8` if upload reports **N8 (No PSRAM)**; otherwise use `display`.

| Action | Command |
|--------|---------|
| Build | `pio run -e display_n8` |
| Upload | `pio run -e display_n8 -t upload` |
| Serial monitor | `pio device monitor -e display_n8` |
| Upload + monitor | `pio run -e display_n8 -t upload && pio device monitor -e display_n8` |

## TFT smoke test (`lvgl_test`)

No Modbus — use this to verify display wiring only.

| Action | Command |
|--------|---------|
| Build | `pio run -e lvgl_test` |
| Upload | `pio run -e lvgl_test -t upload` |
| Clean | `pio run -e lvgl_test -t clean` |
| Clean + build + upload | `pio run -e lvgl_test -t clean && pio run -e lvgl_test -t upload` |

## Other useful commands

| Action | Command |
|--------|---------|
| List connected boards | `pio device list` |
| Clean all environments | `pio run -t clean` |
| Erase flash (full chip) | `pio run -e display -t erase` |
| Verbose build | `pio run -e display -v` |

## Notes

- Use the **USB** port (native CDC), not the UART/CH340 port, with default `platformio.ini` settings.
- Monitor baud rate is **115200** (`monitor_speed` in `platformio.ini`).
- Stop the serial monitor with `Ctrl+C` before uploading again.
