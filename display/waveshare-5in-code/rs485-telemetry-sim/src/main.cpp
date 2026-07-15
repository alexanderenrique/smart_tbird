/*
 * Thunderbird RS-485 Modbus RTU telemetry sweep simulator (ATtiny3226).
 *
 * Acts as Modbus slave ID 1. Temperatures / slow fields update at 1 Hz;
 * RPM sweeps the full range once per second and is refreshed at 10 Hz so the
 * Waveshare display can stress UI update rates.
 *
 * USART0 alt pins: TX=PA1, RX=PA2. DE/RE on PA3.
 */

#include <Arduino.h>
#include <math.h>

#include "pins.h"
#include "modbus_config.h"
#include "telemetry_data.h"

static uint16_t holding_regs[ModbusConfig::REG_COUNT] = {};

// Cached slow-channel values (temps, voltage, AFR, fan) — refreshed at 1 Hz.
static TelemetryData slow_data = {};

static constexpr unsigned long SLOW_UPDATE_MS = 1000;  // 1 Hz
static constexpr unsigned long RPM_UPDATE_MS = 100;   // 10 Hz
static constexpr float RPM_SWEEP_PERIOD_SEC = 1.0f;   // full high–low cycle

// Modbus RTU: baud > 19200 uses fixed T3.5 = 1.75 ms (spec).
static constexpr unsigned long FRAME_GAP_US = 1750;
static constexpr uint8_t FC_READ_HOLDING = 0x03;
static constexpr uint8_t EX_ILLEGAL_FUNCTION = 0x01;
static constexpr uint8_t EX_ILLEGAL_DATA_ADDRESS = 0x02;
static constexpr size_t RX_BUF_SIZE = 64;
static constexpr size_t TX_BUF_SIZE = 64;

static uint8_t rx_buf[RX_BUF_SIZE];
static size_t rx_len = 0;
static unsigned long last_rx_us = 0;

static uint16_t modbusCrc(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static void setTransmit(bool enable) {
    digitalWrite(Pins::MODBUS_DE_RE, enable ? HIGH : LOW);
}

static void sendFrame(const uint8_t *data, size_t len) {
    setTransmit(true);
    // Brief settle so the transceiver fully enables before the start bit.
    delayMicroseconds(50);
    Serial.write(data, len);
    Serial.flush();
    // One char time at 115200 (~100 us) after last stop bit before releasing DE.
    delayMicroseconds(120);
    setTransmit(false);
}

static void sendException(uint8_t slave_id, uint8_t function, uint8_t exception) {
    uint8_t frame[5];
    frame[0] = slave_id;
    frame[1] = static_cast<uint8_t>(function | 0x80);
    frame[2] = exception;
    const uint16_t crc = modbusCrc(frame, 3);
    frame[3] = static_cast<uint8_t>(crc & 0xFF);
    frame[4] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    sendFrame(frame, 5);
}

static void handleReadHolding(const uint8_t *req, size_t len) {
    if (len != 8) {
        return;
    }

    const uint8_t slave_id = req[0];
    const uint16_t address = static_cast<uint16_t>((req[2] << 8) | req[3]);
    const uint16_t words = static_cast<uint16_t>((req[4] << 8) | req[5]);

    if (words == 0 ||
        words > ModbusConfig::REG_COUNT ||
        address >= ModbusConfig::REG_COUNT ||
        (address + words) > ModbusConfig::REG_COUNT) {
        sendException(slave_id, FC_READ_HOLDING, EX_ILLEGAL_DATA_ADDRESS);
        return;
    }

    uint8_t frame[TX_BUF_SIZE];
    const uint8_t byte_count = static_cast<uint8_t>(words * 2);
    frame[0] = slave_id;
    frame[1] = FC_READ_HOLDING;
    frame[2] = byte_count;
    size_t pos = 3;
    for (uint16_t i = 0; i < words; i++) {
        const uint16_t value = holding_regs[address + i];
        frame[pos++] = static_cast<uint8_t>((value >> 8) & 0xFF);
        frame[pos++] = static_cast<uint8_t>(value & 0xFF);
    }
    const uint16_t crc = modbusCrc(frame, pos);
    frame[pos++] = static_cast<uint8_t>(crc & 0xFF);
    frame[pos++] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    sendFrame(frame, pos);
}

static void processFrame(const uint8_t *frame, size_t len) {
    if (len < 4) {
        return;
    }

    const uint16_t rx_crc = static_cast<uint16_t>(frame[len - 2] | (frame[len - 1] << 8));
    if (modbusCrc(frame, len - 2) != rx_crc) {
        return;
    }

    const uint8_t slave_id = frame[0];
    // Ignore other slaves; still accept broadcast address 0 for completeness (no reply needed).
    if (slave_id != ModbusConfig::SLAVE_ID && slave_id != 0) {
        return;
    }

    const uint8_t function = frame[1];
    if (function == FC_READ_HOLDING) {
        if (slave_id == 0) {
            return;  // no response to broadcast reads
        }
        handleReadHolding(frame, len);
        return;
    }

    if (slave_id != 0) {
        sendException(slave_id, function, EX_ILLEGAL_FUNCTION);
    }
}

static void pollModbus() {
    while (Serial.available() > 0) {
        if (rx_len < RX_BUF_SIZE) {
            rx_buf[rx_len++] = static_cast<uint8_t>(Serial.read());
        } else {
            (void)Serial.read();
            rx_len = 0;
        }
        last_rx_us = micros();
    }

    if (rx_len == 0) {
        return;
    }

    const unsigned long idle_us = micros() - last_rx_us;
    if (idle_us < FRAME_GAP_US) {
        return;
    }

    processFrame(rx_buf, rx_len);
    rx_len = 0;
}

static float sweep(float t_sec, float period_sec, float lo, float hi, float phase) {
    const float angle = (t_sec / period_sec) * 2.0f * static_cast<float>(M_PI) + phase;
    const float mid = 0.5f * (lo + hi);
    const float amp = 0.5f * (hi - lo);
    return mid + amp * sinf(angle);
}

static void updateSlowChannels(float t) {
    slow_data.valid = true;
    slow_data.coolant_temp_f = sweep(t, 22.0f, 160.0f, 220.0f, 0.0f);
    slow_data.oil_temp_f = sweep(t, 26.0f, 160.0f, 220.0f, 1.2f);
    slow_data.trans_temp_f = sweep(t, 30.0f, 140.0f, 210.0f, 2.4f);
    slow_data.voltage_v = sweep(t, 20.0f, 12.0f, 14.8f, 0.6f);
    slow_data.afr = sweep(t, 18.0f, 12.5f, 15.5f, 1.8f);
    slow_data.pcb_temp_f = sweep(t, 28.0f, 70.0f, 110.0f, 0.9f);
    slow_data.iat_f = sweep(t, 32.0f, 80.0f, 160.0f, 2.1f);
    slow_data.fan_pwm_pct = static_cast<uint16_t>(lroundf(sweep(t, 16.0f, 0.0f, 100.0f, 3.6f)));
}

static void publishRegisters(float t) {
    TelemetryData data = slow_data;
    data.valid = true;
    data.rpm = static_cast<uint16_t>(lroundf(sweep(t, RPM_SWEEP_PERIOD_SEC, 800.0f, 4500.0f, 0.0f)));
    telemetryToRegisters(data, holding_regs);
}

void setup() {
    pinMode(Pins::MODBUS_DE_RE, OUTPUT);
    setTransmit(false);

    const float t0 = millis() / 1000.0f;
    updateSlowChannels(t0);
    publishRegisters(t0);

    // USART0 ALT1: TX=PA1, RX=PA2 (board DI/RO routing).
    Serial.swap(1);
    Serial.begin(ModbusConfig::BAUD_RATE);
}

void loop() {
    static unsigned long last_slow = 0;
    static unsigned long last_rpm = 0;
    const unsigned long now = millis();
    const float t = now / 1000.0f;

    pollModbus();

    if (last_slow == 0 || (now - last_slow) >= SLOW_UPDATE_MS) {
        updateSlowChannels(t);
        last_slow = now;
    }

    if (last_rpm == 0 || (now - last_rpm) >= RPM_UPDATE_MS) {
        publishRegisters(t);
        last_rpm = now;
    }
}
