# Display (Modbus UI firmware)
pio run -e BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5_B -t upload

# Dummy ESP32 (Modbus slave sim)
cd rs485-telemetry-sim && pio run -e esp32dev -t upload

# Raw RS-485 link test (115200, no Modbus)
cd rs485-link-test && pio run -e esp32dev-slave -t upload
cd rs485-link-test && pio run -e waveshare-master -t upload
