#pragma once

// GPIO assignments for the display board.
// Set each pin when breadboarding is complete. Use -1 until assigned.
//
// ESP32-S3 pins to avoid for general GPIO: 0, 3, 45, 46 (strapping)
// GPIO 43/44 are UART0 (U0TXD/U0RXD) — fine for Modbus UART, but conflict
// with USB CDC serial debug when ARDUINO_USB_CDC_ON_BOOT=1 (use UART1/2 instead,
// or use the board's UART USB port for monitor while 43/44 drive RS-485).

// Set to 1 when RS-485 is wired and ready to test
#define ENABLE_MODBUS_RTU 0

namespace Pins {

// RS-485 Modbus (MAX485) — dummy pins while disabled; must NOT use 43/44 (USB on S3)
constexpr int MODBUS_RX    = -1;
constexpr int MODBUS_TX    = -1;
constexpr int MODBUS_DE_RE = -1;

// Local LDR for auto-dim backlight
constexpr int LDR_ADC = 20;      // was GPIO 4 on ESP32

// Backlight PWM (LEDC) — GPIO HIGH enables backlight (active-high)
constexpr int BACKLIGHT_PWM = 5; // was GPIO 25 on ESP32
constexpr bool BACKLIGHT_ACTIVE_HIGH = true;

// Local Voltage (Vbat)
constexpr int VOLTAGE_ADC = -1;

// PCB board temperature
constexpr int BOARD_TEMP_ADC = -1;

// TFT SPI bus and TOUCH_CS are configured in tft_setup.h (TFT_eSPI macros).
// Touch shares MOSI/SCLK/MISO with the TFT; only T_CS is a separate GPIO.
constexpr int TOUCH_CS_GPIO = 8;  // must match #define TOUCH_CS in tft_setup.h

inline bool touchPinAssigned() {
    return TOUCH_CS_GPIO >= 0;
}

inline bool modbusPinsAssigned() {
    return ENABLE_MODBUS_RTU && MODBUS_RX >= 0 && MODBUS_TX >= 0 && MODBUS_DE_RE >= 0;
}

inline bool ldrPinAssigned() {
    return LDR_ADC >= 0;
}

inline bool backlightPinAssigned() {
    return BACKLIGHT_PWM >= 0;
}

}  // namespace Pins
