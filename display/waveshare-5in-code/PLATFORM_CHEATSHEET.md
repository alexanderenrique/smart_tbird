# Display (Modbus master UI) — Waveshare
pio run -e BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5_B -t upload

# Dummy ATtiny3226 (Modbus slave sim) — RX=PA2 TX=PA1 DE=PA3 @ 115200
cd rs485-telemetry-sim && pio run -e attiny3226
# Flash the resulting firmware.hex via UPDI (attiny-uploader / SerialUPDI)

# Raw RS-485 link test (no Modbus), if needed again
cd rs485-link-test && pio run -e esp32dev-slave -t upload
cd rs485-link-test && pio run -e waveshare-master -t upload
