#pragma once

// Waveshare ESP32-S3-Touch-LCD-5B onboard RS-485 (SP3485, auto DE/RE).
// GPIO 43/44 are UART0 on this board; use USB CDC for serial monitor.

#define ENABLE_MODBUS_RTU 1

namespace Pins {

constexpr int MODBUS_RX = 43;
constexpr int MODBUS_TX = 44;
constexpr int MODBUS_DE_RE = -1;  // auto direction switching on Waveshare

inline bool modbusPinsAssigned() {
    return ENABLE_MODBUS_RTU && MODBUS_RX >= 0 && MODBUS_TX >= 0;
}

}  // namespace Pins
