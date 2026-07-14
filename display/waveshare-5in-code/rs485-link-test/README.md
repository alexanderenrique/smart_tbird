# RS-485 raw link test

Step-back bring-up: no Modbus. Master (Waveshare) blasts ASCII; slave (dummy ESP32) only listens.

Matches Waveshare [`02_RS485_Test`](https://docs.waveshare.com/ESP32-S3-Touch-LCD-5/Development-Environment-Setup-Arduino): **115200 8N1**, Waveshare **GPIO43/44**.

## Wiring

| Waveshare RS-485 | Dummy + MAX485 |
|------------------|----------------|
| A                | A              |
| B                | B              |
| GND              | GND            |

Dummy pins: RX=16, TX=17, DE+RE=4 (held **LOW** = receive).

## Flash

```bash
cd display/waveshare-5in-code/rs485-link-test

# 1) Dummy ESP32 — listen
pio run -e esp32dev-slave -t upload
pio device monitor -e esp32dev-slave

# 2) Waveshare display — send
pio run -e waveshare-master -t upload
pio device monitor -e waveshare-master
```

## Success

Slave monitor prints a line every second:

```text
RS485 TEST 1
RS485 TEST 2
...
```

Master monitor prints matching `TX [n]: RS485 TEST n`. If the slave stays silent but the master shows TX, probe A/B and the dummy RX pin next.
