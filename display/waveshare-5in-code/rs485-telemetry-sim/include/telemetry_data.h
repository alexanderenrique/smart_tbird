#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "modbus_config.h"

struct TelemetryData {
    bool valid;
    float coolant_temp_f;
    float oil_temp_f;
    float trans_temp_f;
    float voltage_v;
    float afr;
    uint16_t rpm;
    float pcb_temp_f;
    float iat_f;
    uint16_t fan_pwm_pct;
};

inline void telemetryToRegisters(const TelemetryData &data, uint16_t *regs) {
    if (regs == nullptr) {
        return;
    }
    regs[ModbusConfig::REG_COOLANT_TEMP] = ModbusConfig::tempToRegister(data.coolant_temp_f);
    regs[ModbusConfig::REG_OIL_TEMP] = ModbusConfig::tempToRegister(data.oil_temp_f);
    regs[ModbusConfig::REG_TRANS_TEMP] = ModbusConfig::tempToRegister(data.trans_temp_f);
    regs[ModbusConfig::REG_VOLTAGE] = ModbusConfig::voltageToRegister(data.voltage_v);
    regs[ModbusConfig::REG_AFR] = ModbusConfig::afrToRegister(data.afr);
    regs[ModbusConfig::REG_RPM] = data.rpm;
    regs[ModbusConfig::REG_PCB_TEMP] = ModbusConfig::tempToRegister(data.pcb_temp_f);
    regs[ModbusConfig::REG_IAT] = ModbusConfig::tempToRegister(data.iat_f);
    regs[ModbusConfig::REG_FAN_PWM] = data.fan_pwm_pct;
}
