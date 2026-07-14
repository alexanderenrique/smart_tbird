#pragma once

// Modbus RTU holding-register map for the remote sensor / sweep board.
// Keep in sync with rs485-telemetry-sim/include/modbus_config.h

#include <stdint.h>

namespace ModbusConfig {

constexpr uint8_t SLAVE_ID = 1;
constexpr uint32_t BAUD_RATE = 115200;  // Waveshare RS-485 demos / link test
constexpr uint32_t TIMEOUT_MS = 2000;
constexpr uint32_t POLL_INTERVAL_MS = 500;

constexpr uint16_t REG_START = 0;
constexpr uint16_t REG_COUNT = 9;

// Register indices (relative to REG_START)
constexpr uint16_t REG_COOLANT_TEMP = 0;  // tenths °F (1850 = 185.0°F)
constexpr uint16_t REG_OIL_TEMP     = 1;
constexpr uint16_t REG_TRANS_TEMP   = 2;
constexpr uint16_t REG_VOLTAGE      = 3;  // hundredths V (1420 = 14.20 V)
constexpr uint16_t REG_AFR          = 4;  // hundredths (1470 = 14.70)
constexpr uint16_t REG_RPM          = 5;  // integer RPM
constexpr uint16_t REG_PCB_TEMP     = 6;  // tenths °F
constexpr uint16_t REG_IAT          = 7;  // tenths °F
constexpr uint16_t REG_FAN_PWM      = 8;  // 0–100 %

inline float tempFromRegister(uint16_t raw) {
    return raw / 10.0f;
}

inline uint16_t tempToRegister(float temp_f) {
    if (temp_f < 0.0f) {
        return 0;
    }
    return static_cast<uint16_t>(temp_f * 10.0f + 0.5f);
}

inline float voltageFromRegister(uint16_t raw) {
    return raw / 100.0f;
}

inline uint16_t voltageToRegister(float volts) {
    if (volts < 0.0f) {
        return 0;
    }
    return static_cast<uint16_t>(volts * 100.0f + 0.5f);
}

inline float afrFromRegister(uint16_t raw) {
    return raw / 100.0f;
}

inline uint16_t afrToRegister(float afr) {
    if (afr < 0.0f) {
        return 0;
    }
    return static_cast<uint16_t>(afr * 100.0f + 0.5f);
}

}  // namespace ModbusConfig
