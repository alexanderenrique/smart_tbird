#pragma once

// Modbus RTU holding-register map for the remote sensor board.
// Both the display (client) and sensor board (slave) must use the same map.

namespace ModbusConfig {

constexpr uint8_t SLAVE_ID = 1;
constexpr uint32_t BAUD_RATE = 9600;
constexpr uint32_t TIMEOUT_MS = 2000;
constexpr uint32_t POLL_INTERVAL_MS = 500;

// First holding register address and count for a single block read
constexpr uint16_t REG_START = 0;
constexpr uint16_t REG_COUNT = 5;

// Register indices (relative to REG_START)
constexpr uint16_t REG_COOLANT_TEMP = 0;  // tenths of °F (185 = 18.5°F)
constexpr uint16_t REG_OIL_TEMP     = 1;
constexpr uint16_t REG_TRANS_TEMP   = 2;
constexpr uint16_t REG_VOLTAGE      = 3;  // hundredths of V (1388 = 13.88 V)
constexpr uint16_t REG_AFR          = 4;  // hundredths (1330 = 13.30)

inline float tempFromRegister(uint16_t raw) {
    return raw / 10.0f;
}

inline float voltageFromRegister(uint16_t raw) {
    return raw / 100.0f;
}

inline float afrFromRegister(uint16_t raw) {
    return raw / 100.0f;
}

}  // namespace ModbusConfig
