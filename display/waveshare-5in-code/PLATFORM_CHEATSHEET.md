# Display (Modbus master UI) — Waveshare
pio run -e BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5_B -t upload

# Dummy ESP32 (Modbus slave sim) — RX=16 TX=17 DE=4 @ 115200
cd rs485-telemetry-sim && pio run -e esp32dev -t upload

# Raw RS-485 link test (no Modbus), if needed again
cd rs485-link-test && pio run -e esp32dev-slave -t upload
cd rs485-link-test && pio run -e waveshare-master -t upload
