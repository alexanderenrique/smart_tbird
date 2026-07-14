/*
 * Thunderbird RS-485 Modbus RTU telemetry sweep simulator.
 *
 * Acts as Modbus slave ID 1 and slowly sweeps all holding registers so the
 * Waveshare display can exercise live UI updates over the bus.
 */

#include <Arduino.h>
#include <math.h>

#include <ModbusServerRTU.h>
#include <RTUutils.h>

#include "pins.h"
#include "modbus_config.h"
#include "telemetry_data.h"

static HardwareSerial RS485(2);
static ModbusServerRTU MBserver(ModbusConfig::TIMEOUT_MS, Pins::MODBUS_DE_RE);

static uint16_t holding_regs[ModbusConfig::REG_COUNT] = {};

static float sweep(float t_sec, float period_sec, float lo, float hi, float phase) {
    const float angle = (t_sec / period_sec) * 2.0f * static_cast<float>(M_PI) + phase;
    const float mid = 0.5f * (lo + hi);
    const float amp = 0.5f * (hi - lo);
    return mid + amp * sinf(angle);
}

static void updateSweeps() {
    const float t = millis() / 1000.0f;

    TelemetryData data = {};
    data.valid = true;
    data.coolant_temp_f = sweep(t, 22.0f, 160.0f, 220.0f, 0.0f);
    data.oil_temp_f = sweep(t, 26.0f, 160.0f, 220.0f, 1.2f);
    data.trans_temp_f = sweep(t, 30.0f, 140.0f, 210.0f, 2.4f);
    data.voltage_v = sweep(t, 20.0f, 12.0f, 14.8f, 0.6f);
    data.afr = sweep(t, 18.0f, 12.5f, 15.5f, 1.8f);
    data.rpm = static_cast<uint16_t>(lroundf(sweep(t, 24.0f, 800.0f, 4500.0f, 3.0f)));
    data.pcb_temp_f = sweep(t, 28.0f, 70.0f, 110.0f, 0.9f);
    data.iat_f = sweep(t, 32.0f, 80.0f, 160.0f, 2.1f);
    data.fan_pwm_pct = static_cast<uint16_t>(lroundf(sweep(t, 16.0f, 0.0f, 100.0f, 3.6f)));

    telemetryToRegisters(data, holding_regs);
}

// FC03: READ_HOLD_REGISTER — address 0 is valid (REG_START = 0).
static ModbusMessage FC03(ModbusMessage request) {
    uint16_t address = 0;
    uint16_t words = 0;
    ModbusMessage response;

    request.get(2, address);
    request.get(4, words);

    if (words == 0 ||
        words > ModbusConfig::REG_COUNT ||
        address >= ModbusConfig::REG_COUNT ||
        (address + words) > ModbusConfig::REG_COUNT) {
        response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
        return response;
    }

    response.add(request.getServerID(), request.getFunctionCode(),
                 static_cast<uint8_t>(words * 2));
    for (uint16_t i = 0; i < words; i++) {
        response.add(holding_regs[address + i]);
    }
    return response;
}

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("RS-485 telemetry sweep simulator");

    updateSweeps();

    RTUutils::prepareHardwareSerial(RS485);
    RS485.begin(ModbusConfig::BAUD_RATE, SERIAL_8N1, Pins::MODBUS_RX, Pins::MODBUS_TX);

    MBserver.registerWorker(ModbusConfig::SLAVE_ID, READ_HOLD_REGISTER, &FC03);
    MBserver.begin(RS485);

    Serial.printf("Modbus slave %u @ %lu baud  RX=%d TX=%d DE=%d\n",
                  ModbusConfig::SLAVE_ID,
                  static_cast<unsigned long>(ModbusConfig::BAUD_RATE),
                  Pins::MODBUS_RX, Pins::MODBUS_TX, Pins::MODBUS_DE_RE);
    Serial.println("Sweeping holding regs 0–8");
}

void loop() {
    static unsigned long last_update = 0;
    if (millis() - last_update >= 50) {
        updateSweeps();
        last_update = millis();
    }
    delay(5);
}
