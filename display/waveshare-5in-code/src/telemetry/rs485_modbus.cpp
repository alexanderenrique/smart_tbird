#include "rs485_modbus.h"

#include <Arduino.h>
#include <ModbusClientRTU.h>
#include <RTUutils.h>

#include "pins.h"
#include "modbus_config.h"

#if ENABLE_MODBUS_RTU

static HardwareSerial RS485(1);
static ModbusClientRTU MBclient(Pins::MODBUS_DE_RE);

static uint16_t modbus_regs[ModbusConfig::REG_COUNT] = {};
static volatile bool modbus_data_valid = false;
static volatile bool modbus_data_fresh = false;

static void handleModbusData(ModbusMessage response, uint32_t token) {
    (void)token;
    if (response.getFunctionCode() != READ_HOLD_REGISTER) {
        return;
    }

    const uint8_t byteCount = response[2];
    const unsigned neededBytes = ModbusConfig::REG_COUNT * 2;
    if (byteCount < neededBytes || response.size() < 3 + neededBytes) {
        return;
    }

    for (unsigned i = 0; i < ModbusConfig::REG_COUNT; i++) {
        const unsigned idx = 3 + (i * 2);
        modbus_regs[i] = (static_cast<uint16_t>(response[idx]) << 8) | response[idx + 1];
    }
    modbus_data_valid = true;
    modbus_data_fresh = true;
}

static void handleModbusError(Error error, uint32_t token) {
    (void)token;
    Serial.printf("Modbus error: %02X\n", static_cast<unsigned>(error));
    modbus_data_valid = false;
}

void initRs485() {
    if (!Pins::modbusPinsAssigned()) {
        Serial.println("WARN: Modbus pins not assigned — skipping RS-485 init");
        return;
    }

    RTUutils::prepareHardwareSerial(RS485);
    RS485.begin(ModbusConfig::BAUD_RATE, SERIAL_8N1, Pins::MODBUS_RX, Pins::MODBUS_TX);

    MBclient.begin(RS485);
    MBclient.setTimeout(ModbusConfig::TIMEOUT_MS);
    MBclient.onDataHandler(&handleModbusData);
    MBclient.onErrorHandler(&handleModbusError);

    Serial.printf("Modbus RTU client ready (RX=%d TX=%d DE=%d @ %lu baud)\n",
                  Pins::MODBUS_RX, Pins::MODBUS_TX, Pins::MODBUS_DE_RE,
                  static_cast<unsigned long>(ModbusConfig::BAUD_RATE));
}

void pollRs485() {
    if (!Pins::modbusPinsAssigned()) {
        return;
    }

    static unsigned long last_request = 0;
    static uint32_t request_token = 1;

    if (millis() - last_request < ModbusConfig::POLL_INTERVAL_MS) {
        return;
    }

    Error err = MBclient.addRequest(
        request_token++,
        ModbusConfig::SLAVE_ID,
        READ_HOLD_REGISTER,
        ModbusConfig::REG_START,
        ModbusConfig::REG_COUNT);
    if (err != SUCCESS) {
        Serial.printf("Failed to add Modbus request: %02X\n", static_cast<unsigned>(err));
    }
    last_request = millis();
}

bool rs485TakeFreshTelemetry(TelemetryData *out) {
    if (out == nullptr || !modbus_data_fresh) {
        return false;
    }
    modbus_data_fresh = false;

    // Snapshot registers under a brief critical section of booleans only;
    // eModbus callbacks run on another task, so copy once.
    uint16_t snapshot[ModbusConfig::REG_COUNT];
    noInterrupts();
    for (unsigned i = 0; i < ModbusConfig::REG_COUNT; i++) {
        snapshot[i] = modbus_regs[i];
    }
    const bool valid = modbus_data_valid;
    interrupts();

    *out = telemetryFromRegisters(snapshot, valid);
    return valid;
}

#else  // !ENABLE_MODBUS_RTU

void initRs485() {}
void pollRs485() {}

bool rs485TakeFreshTelemetry(TelemetryData *out) {
    (void)out;
    return false;
}

#endif
