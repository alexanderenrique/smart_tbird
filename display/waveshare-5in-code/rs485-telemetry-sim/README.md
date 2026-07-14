# RS-485 Telemetry Sweep Simulator

Dummy Modbus RTU **slave** for bringing up the Waveshare ESP32-S3-Touch-LCD-5B display over RS-485. It publishes the same holding-register map as the display firmware and slowly sweeps every value with staggered phases.

## Wiring

| Waveshare RS-485 terminal | Dummy ESP32 + MAX485 |
|---------------------------|----------------------|
| A                         | A                    |
| B                         | B                    |
| GND                       | GND                  |

Default MAX485 pins (edit [`include/pins.h`](include/pins.h)):

| Signal | GPIO |
|--------|------|
| RO → RX | 16 |
| DI → TX | 17 |
| DE+RE   | 4 |

On the Waveshare board, RS-485 is onboard (GPIO 43/44, auto DE/RE). Enable the 120 Ω termination jumper if the bus is short or noisy.

## Protocol

- Slave ID **1**, **115200** 8N1 (same as Waveshare `02_RS485_Test`)
- Holding registers **0–8** (see map below)

| Reg | Field | Encoding | Sweep range |
|-----|-------|----------|-------------|
| 0 | Coolant °F | tenths | 160–220 |
| 1 | Oil °F | tenths | 160–220 |
| 2 | Trans °F | tenths | 140–210 |
| 3 | Voltage V | hundredths | 12.0–14.8 |
| 4 | AFR | hundredths | 12.5–15.5 |
| 5 | RPM | integer | 800–4500 |
| 6 | PCB °F | tenths | 70–110 |
| 7 | IAT °F | tenths | 80–160 |
| 8 | Fan PWM % | 0–100 | 0–100 |

Keep [`include/modbus_config.h`](include/modbus_config.h) in sync with [`../src/telemetry/modbus_config.h`](../src/telemetry/modbus_config.h).

## Build and flash

```bash
cd display/waveshare-5in-code/rs485-telemetry-sim

# Classic ESP32 DevKit
pio run -e esp32dev -t upload
pio device monitor -e esp32dev

# ESP32-S3 DevKit
pio run -e esp32-s3-devkitc-1 -t upload
```

Flash the display firmware from the parent project, then confirm gauges move through their ranges.
