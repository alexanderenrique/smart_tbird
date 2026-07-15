# RS-485 Telemetry Sweep Simulator (ATtiny3226)

Dummy Modbus RTU **slave** for bringing up the Waveshare ESP32-S3-Touch-LCD-5B display over RS-485. It publishes the same holding-register map as the display firmware. Temperatures and other slow fields update at **1 Hz**; **RPM** runs a full high–low sine sweep every **1 s** and is refreshed at **10 Hz** (display poll interval matches).

## Wiring

| Waveshare RS-485 terminal | ATtiny3226 + MAX485 |
|---------------------------|---------------------|
| A                         | A                   |
| B                         | B                   |
| GND                       | GND                 |

USART0 **ALT1** pins (board packaging; `Serial.swap(1)`):

| Signal | ATtiny pin | Arduino # | MAX485 |
|--------|------------|-----------|--------|
| TX     | **PA1**    | 14        | DI     |
| RX     | **PA2**    | 15        | RO     |
| DE+RE  | **PA3**    | 16        | DE+RE (tied) |
| UPDI   | PA0        | 17        | (programmer) |

Default USART0 mux (PB2/PB3) is unused on this board. Edit [`include/pins.h`](include/pins.h) if wiring changes.

On the Waveshare board, RS-485 is onboard (GPIO 43/44, auto DE/RE). Enable the 120 Ω termination jumper if the bus is short or noisy.

## Protocol

- Slave ID **1**, **115200** 8N1 (same as Waveshare `02_RS485_Test`)
- Holding registers **0–8** (see map below)
- Function code **03** (Read Holding Registers) only

| Reg | Field | Encoding | Sweep range |
|-----|-------|----------|-------------|
| 0 | Coolant °F | tenths | 160–220 |
| 1 | Oil °F | tenths | 160–220 |
| 2 | Trans °F | tenths | 140–210 |
| 3 | Voltage V | hundredths | 12.0–14.8 |
| 4 | AFR | hundredths | 12.5–15.5 |
| 5 | RPM | integer | 800–4500 (1 s full sweep @ 10 Hz) |
| 6 | PCB °F | tenths | 70–110 |
| 7 | IAT °F | tenths | 80–160 |
| 8 | Fan PWM % | 0–100 | 0–100 |

Keep [`include/modbus_config.h`](include/modbus_config.h) in sync with [`../src/telemetry/modbus_config.h`](../src/telemetry/modbus_config.h).

## Build and flash

```bash
cd display/waveshare-5in-code/rs485-telemetry-sim
pio run -e attiny3226
```

Intel HEX is written to:

- `.pio/build/attiny3226/firmware.hex`
- `firmware.hex` (project root copy, for UPDI tools)

Example with the repo UPDI programmer:

```bash
attiny-uploader --port /dev/cu.SLAB_USBtoUART --verify firmware.hex
```

Flash the display firmware from the parent project, then confirm gauges move through their ranges.
