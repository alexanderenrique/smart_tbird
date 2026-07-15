#include <Arduino.h>
#include <math.h>

namespace Pins {
constexpr uint8_t COOLANT_TEMP = PIN_PC0;
constexpr uint8_t OIL_TEMP = PIN_PC1;
constexpr uint8_t TRANS_TEMP = PIN_PC2;
constexpr uint8_t IAT_TEMP = PIN_PC3;
constexpr uint8_t PCB_TEMP = PIN_PA5;
constexpr uint8_t AFR = PIN_PA6;
constexpr uint8_t SYSTEM_VOLTAGE = PIN_PB1;
constexpr uint8_t RPM = PIN_PB4;
constexpr uint8_t FAN_PWM = PIN_PA4;
constexpr uint8_t MODBUS_DE_RE = PIN_PA3;
}  // namespace Pins

namespace Config {
constexpr uint8_t MODBUS_SLAVE_ID = 1;
constexpr uint32_t MODBUS_BAUD = 115200;
constexpr uint32_t MODBUS_FRAME_GAP_US = 1750;

constexpr uint32_t ADC_SAMPLE_INTERVAL_MS = 100;
constexpr uint32_t SLOW_PUBLISH_INTERVAL_MS = 1000;
constexpr uint32_t FAST_PUBLISH_INTERVAL_MS = 100;
constexpr uint8_t ADC_AVERAGE_SAMPLES = 8;
constexpr float ADC_REFERENCE_V = 3.3f;
constexpr float ADC_FULL_SCALE = 4095.0f;

constexpr float TEMP_FILTER_ALPHA = 0.10f;
constexpr float AFR_FILTER_ALPHA = 0.35f;
constexpr float RPM_FILTER_ALPHA = 0.15f;
constexpr uint16_t RPM_ROUNDING_STEP = 50;
constexpr float THERMISTOR_FIXED_OHM = 2000.0f;
constexpr float THERMISTOR_NOMINAL_OHM = 10000.0f;
constexpr float THERMISTOR_NOMINAL_K = 298.15f;
constexpr float THERMISTOR_BETA = 3950.0f;

constexpr float VOLTAGE_DIVIDER_RATIO = 6.0f;
constexpr float AFR_MIN = 10.0f;
constexpr float AFR_MAX = 20.0f;

constexpr float FAN_OFF_TEMP_F = 150.0f;
constexpr float FAN_FULL_TEMP_F = 180.0f;

// One ignition pulse every two crankshaft revolutions on one lead of a
// four-stroke engine.
constexpr uint32_t RPM_PERIOD_NUMERATOR = 120000000UL;
constexpr uint32_t RPM_MIN_PERIOD_US = 15000UL;
constexpr uint32_t RPM_TIMEOUT_US = 2000000UL;
}  // namespace Config

namespace Registers {
enum Index : uint8_t {
    COOLANT_TEMP = 0,
    OIL_TEMP = 1,
    TRANS_TEMP = 2,
    VOLTAGE = 3,
    AFR = 4,
    RPM = 5,
    PCB_TEMP = 6,
    IAT = 7,
    FAN_PWM = 8,
    COUNT = 9
};
}  // namespace Registers

struct FilteredTemperature {
    uint8_t pin;
    float value_f;
    bool initialized;
};

static FilteredTemperature temperatures[] = {
    {Pins::COOLANT_TEMP, 0.0f, false},
    {Pins::OIL_TEMP, 0.0f, false},
    {Pins::TRANS_TEMP, 0.0f, false},
    {Pins::IAT_TEMP, 0.0f, false},
    {Pins::PCB_TEMP, 0.0f, false},
};

enum TemperatureIndex : uint8_t {
    TEMP_COOLANT = 0,
    TEMP_OIL = 1,
    TEMP_TRANS = 2,
    TEMP_IAT = 3,
    TEMP_PCB = 4
};

static uint16_t holding_regs[Registers::COUNT] = {};
static float filtered_afr = Config::AFR_MIN;
static bool afr_initialized = false;

static volatile uint32_t rpm_periods_us[3] = {};
static volatile uint8_t rpm_period_index = 0;
static volatile bool rpm_periods_valid = false;
static volatile uint32_t last_rpm_edge_us = 0;
static float filtered_rpm = 0.0f;
static bool rpm_filter_initialized = false;

static constexpr uint8_t FC_READ_HOLDING = 0x03;
static constexpr uint8_t EX_ILLEGAL_FUNCTION = 0x01;
static constexpr uint8_t EX_ILLEGAL_DATA_ADDRESS = 0x02;
static constexpr size_t MODBUS_RX_BUFFER_SIZE = 64;
static constexpr size_t MODBUS_TX_BUFFER_SIZE = 32;

static uint8_t modbus_rx_buffer[MODBUS_RX_BUFFER_SIZE];
static size_t modbus_rx_length = 0;
static uint32_t modbus_last_rx_us = 0;

static uint16_t clampToRegister(float value) {
    if (value <= 0.0f) {
        return 0;
    }
    if (value >= 65535.0f) {
        return 65535;
    }
    return static_cast<uint16_t>(lroundf(value));
}

static float readAveragedAdc(uint8_t pin) {
    uint32_t sum = 0;
    for (uint8_t i = 0; i < Config::ADC_AVERAGE_SAMPLES; ++i) {
        sum += static_cast<uint16_t>(analogRead(pin));
    }
    return static_cast<float>(sum) / Config::ADC_AVERAGE_SAMPLES;
}

static float adcToTemperatureF(float adc) {
    // The thermistor is the bottom leg: Vout/VDD = Rntc/(2k + Rntc).
    if (adc < 1.0f) {
        adc = 1.0f;
    } else if (adc > Config::ADC_FULL_SCALE - 1.0f) {
        adc = Config::ADC_FULL_SCALE - 1.0f;
    }

    const float resistance =
        Config::THERMISTOR_FIXED_OHM * adc / (Config::ADC_FULL_SCALE - adc);
    const float inverse_kelvin =
        (1.0f / Config::THERMISTOR_NOMINAL_K) +
        (logf(resistance / Config::THERMISTOR_NOMINAL_OHM) /
         Config::THERMISTOR_BETA);
    const float temp_c = (1.0f / inverse_kelvin) - 273.15f;
    return temp_c * 1.8f + 32.0f;
}

static void sampleTemperatures() {
    for (FilteredTemperature &temperature : temperatures) {
        const float sample_f = adcToTemperatureF(readAveragedAdc(temperature.pin));
        if (!temperature.initialized) {
            temperature.value_f = sample_f;
            temperature.initialized = true;
        } else {
            temperature.value_f +=
                Config::TEMP_FILTER_ALPHA * (sample_f - temperature.value_f);
        }
    }
}

static void sampleAfr() {
    const float adc = readAveragedAdc(Pins::AFR);
    const float sample =
        Config::AFR_MIN +
        (adc / Config::ADC_FULL_SCALE) * (Config::AFR_MAX - Config::AFR_MIN);

    if (!afr_initialized) {
        filtered_afr = sample;
        afr_initialized = true;
    } else {
        filtered_afr += Config::AFR_FILTER_ALPHA * (sample - filtered_afr);
    }
}

static uint8_t coolantFanPercent(float coolant_temp_f) {
    if (coolant_temp_f <= Config::FAN_OFF_TEMP_F) {
        return 0;
    }
    if (coolant_temp_f >= Config::FAN_FULL_TEMP_F) {
        return 100;
    }

    const float span = Config::FAN_FULL_TEMP_F - Config::FAN_OFF_TEMP_F;
    return static_cast<uint8_t>(
        lroundf((coolant_temp_f - Config::FAN_OFF_TEMP_F) * 100.0f / span));
}

static void publishSlowRegisters() {
    holding_regs[Registers::COOLANT_TEMP] =
        clampToRegister(temperatures[TEMP_COOLANT].value_f * 10.0f);
    holding_regs[Registers::OIL_TEMP] =
        clampToRegister(temperatures[TEMP_OIL].value_f * 10.0f);
    holding_regs[Registers::TRANS_TEMP] =
        clampToRegister(temperatures[TEMP_TRANS].value_f * 10.0f);
    holding_regs[Registers::PCB_TEMP] =
        clampToRegister(temperatures[TEMP_PCB].value_f * 10.0f);
    holding_regs[Registers::IAT] =
        clampToRegister(temperatures[TEMP_IAT].value_f * 10.0f);

    const float system_voltage =
        (readAveragedAdc(Pins::SYSTEM_VOLTAGE) / Config::ADC_FULL_SCALE) *
        Config::ADC_REFERENCE_V * Config::VOLTAGE_DIVIDER_RATIO;
    holding_regs[Registers::VOLTAGE] = clampToRegister(system_voltage * 100.0f);

    const uint8_t fan_percent =
        coolantFanPercent(temperatures[TEMP_COOLANT].value_f);
    analogWrite(Pins::FAN_PWM, (static_cast<uint16_t>(fan_percent) * 255U) / 100U);
    holding_regs[Registers::FAN_PWM] = fan_percent;
}

static void rpmEdgeHandler() {
    const uint32_t now_us = micros();
    const uint32_t previous_edge_us = last_rpm_edge_us;

    if (previous_edge_us == 0) {
        last_rpm_edge_us = now_us;
        return;
    }

    const uint32_t period_us = now_us - previous_edge_us;
    if (period_us <= Config::RPM_MIN_PERIOD_US) {
        // Ignore implausibly fast triggers without advancing the reference
        // edge, so noise cannot corrupt the next legitimate period.
        return;
    }
    last_rpm_edge_us = now_us;

    if (!rpm_periods_valid) {
        // Seed all slots from the first complete pulse period. Subsequent
        // pulses then form a normal rolling three-measurement median.
        rpm_periods_us[0] = period_us;
        rpm_periods_us[1] = period_us;
        rpm_periods_us[2] = period_us;
        rpm_period_index = 0;
        rpm_periods_valid = true;
        return;
    }

    rpm_periods_us[rpm_period_index] = period_us;
    rpm_period_index = static_cast<uint8_t>((rpm_period_index + 1U) % 3U);
}

static uint16_t filteredRpm() {
    uint32_t periods[3];
    uint32_t last_edge_us;
    bool valid;

    noInterrupts();
    periods[0] = rpm_periods_us[0];
    periods[1] = rpm_periods_us[1];
    periods[2] = rpm_periods_us[2];
    last_edge_us = last_rpm_edge_us;
    valid = rpm_periods_valid;
    interrupts();

    if (!valid || (micros() - last_edge_us) > Config::RPM_TIMEOUT_US) {
        rpm_filter_initialized = false;
        filtered_rpm = 0.0f;
        return 0;
    }

    if (periods[0] > periods[1]) {
        const uint32_t swap = periods[0];
        periods[0] = periods[1];
        periods[1] = swap;
    }
    if (periods[1] > periods[2]) {
        const uint32_t swap = periods[1];
        periods[1] = periods[2];
        periods[2] = swap;
    }
    if (periods[0] > periods[1]) {
        const uint32_t swap = periods[0];
        periods[0] = periods[1];
        periods[1] = swap;
    }

    // Period and RPM are inversely related, but the middle of three remains
    // the middle after inversion, so median period yields median RPM.
    const float median_rpm =
        static_cast<float>(Config::RPM_PERIOD_NUMERATOR) /
        static_cast<float>(periods[1]);
    if (!rpm_filter_initialized) {
        filtered_rpm = median_rpm;
        rpm_filter_initialized = true;
    } else {
        filtered_rpm +=
            Config::RPM_FILTER_ALPHA * (median_rpm - filtered_rpm);
    }

    const uint32_t rounded_rpm =
        (static_cast<uint32_t>(filtered_rpm) +
         (Config::RPM_ROUNDING_STEP / 2U)) /
        Config::RPM_ROUNDING_STEP * Config::RPM_ROUNDING_STEP;
    return rounded_rpm > 65535UL ? 65535U
                                 : static_cast<uint16_t>(rounded_rpm);
}

static uint16_t modbusCrc(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc & 1U) ? static_cast<uint16_t>((crc >> 1) ^ 0xA001)
                             : static_cast<uint16_t>(crc >> 1);
        }
    }
    return crc;
}

static void setModbusTransmit(bool enabled) {
    digitalWrite(Pins::MODBUS_DE_RE, enabled ? HIGH : LOW);
}

static void sendModbusFrame(const uint8_t *data, size_t length) {
    setModbusTransmit(true);
    delayMicroseconds(50);
    Serial.write(data, length);
    Serial.flush();
    delayMicroseconds(120);
    setModbusTransmit(false);
}

static void sendModbusException(uint8_t slave_id, uint8_t function,
                                uint8_t exception) {
    uint8_t response[5] = {
        slave_id, static_cast<uint8_t>(function | 0x80U), exception, 0, 0};
    const uint16_t crc = modbusCrc(response, 3);
    response[3] = static_cast<uint8_t>(crc);
    response[4] = static_cast<uint8_t>(crc >> 8);
    sendModbusFrame(response, sizeof(response));
}

static void handleReadHoldingRegisters(const uint8_t *request, size_t length) {
    if (length != 8) {
        return;
    }

    const uint16_t address =
        static_cast<uint16_t>((request[2] << 8) | request[3]);
    const uint16_t count =
        static_cast<uint16_t>((request[4] << 8) | request[5]);

    if (count == 0 || address >= Registers::COUNT ||
        count > Registers::COUNT || (address + count) > Registers::COUNT) {
        sendModbusException(request[0], request[1], EX_ILLEGAL_DATA_ADDRESS);
        return;
    }

    uint8_t response[MODBUS_TX_BUFFER_SIZE];
    size_t position = 0;
    response[position++] = request[0];
    response[position++] = request[1];
    response[position++] = static_cast<uint8_t>(count * 2U);
    for (uint16_t i = 0; i < count; ++i) {
        const uint16_t value = holding_regs[address + i];
        response[position++] = static_cast<uint8_t>(value >> 8);
        response[position++] = static_cast<uint8_t>(value);
    }
    const uint16_t crc = modbusCrc(response, position);
    response[position++] = static_cast<uint8_t>(crc);
    response[position++] = static_cast<uint8_t>(crc >> 8);
    sendModbusFrame(response, position);
}

static void processModbusFrame(const uint8_t *frame, size_t length) {
    if (length < 4) {
        return;
    }

    const uint16_t received_crc =
        static_cast<uint16_t>(frame[length - 2] | (frame[length - 1] << 8));
    if (modbusCrc(frame, length - 2) != received_crc) {
        return;
    }

    if (frame[0] != Config::MODBUS_SLAVE_ID && frame[0] != 0) {
        return;
    }

    // Modbus broadcasts never receive a response. Function 0x03 broadcasts
    // also have no useful effect because this device's registers are read-only.
    if (frame[0] == 0) {
        return;
    }

    if (frame[1] == FC_READ_HOLDING) {
        handleReadHoldingRegisters(frame, length);
    } else {
        sendModbusException(frame[0], frame[1], EX_ILLEGAL_FUNCTION);
    }
}

static void pollModbus() {
    while (Serial.available() > 0) {
        const uint8_t byte = static_cast<uint8_t>(Serial.read());
        if (modbus_rx_length < MODBUS_RX_BUFFER_SIZE) {
            modbus_rx_buffer[modbus_rx_length++] = byte;
        } else {
            modbus_rx_length = 0;
        }
        modbus_last_rx_us = micros();
    }

    if (modbus_rx_length != 0 &&
        (micros() - modbus_last_rx_us) >= Config::MODBUS_FRAME_GAP_US) {
        processModbusFrame(modbus_rx_buffer, modbus_rx_length);
        modbus_rx_length = 0;
    }
}

void setup() {
    pinMode(Pins::MODBUS_DE_RE, OUTPUT);
    setModbusTransmit(false);
    pinMode(Pins::FAN_PWM, OUTPUT);
    analogWrite(Pins::FAN_PWM, 0);

    pinMode(Pins::COOLANT_TEMP, INPUT);
    pinMode(Pins::OIL_TEMP, INPUT);
    pinMode(Pins::TRANS_TEMP, INPUT);
    pinMode(Pins::IAT_TEMP, INPUT);
    pinMode(Pins::PCB_TEMP, INPUT);
    pinMode(Pins::AFR, INPUT);
    pinMode(Pins::SYSTEM_VOLTAGE, INPUT);
    pinMode(Pins::RPM, INPUT);

    analogReference(VDD);
    analogReadResolution(12);
    sampleTemperatures();
    sampleAfr();
    publishSlowRegisters();
    holding_regs[Registers::AFR] = clampToRegister(filtered_afr * 100.0f);
    holding_regs[Registers::RPM] = 0;

    attachInterrupt(digitalPinToInterrupt(Pins::RPM), rpmEdgeHandler, RISING);

    // USART0 ALT1 routes TX to PA1 and RX to PA2.
    Serial.swap(1);
    Serial.begin(Config::MODBUS_BAUD);
}

void loop() {
    static uint32_t last_adc_sample_ms = millis();
    static uint32_t last_slow_publish_ms = millis();
    static uint32_t last_fast_publish_ms = millis();
    const uint32_t now_ms = millis();

    pollModbus();

    if ((now_ms - last_adc_sample_ms) >= Config::ADC_SAMPLE_INTERVAL_MS) {
        last_adc_sample_ms += Config::ADC_SAMPLE_INTERVAL_MS;
        sampleTemperatures();
        sampleAfr();
    }

    if ((now_ms - last_fast_publish_ms) >= Config::FAST_PUBLISH_INTERVAL_MS) {
        last_fast_publish_ms += Config::FAST_PUBLISH_INTERVAL_MS;
        holding_regs[Registers::AFR] = clampToRegister(filtered_afr * 100.0f);
        holding_regs[Registers::RPM] = filteredRpm();
    }

    if ((now_ms - last_slow_publish_ms) >=
        Config::SLOW_PUBLISH_INTERVAL_MS) {
        last_slow_publish_ms += Config::SLOW_PUBLISH_INTERVAL_MS;
        publishSlowRegisters();
    }
}
