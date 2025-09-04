/*
 * INA219 High-Side Current/Power Monitor Sensor Implementation
 * ==========================================================
 * 
 * Implementation of the INA219 sensor class for battery voltage and current monitoring.
 */

#include "ina219_sensor.h"

// Constructor
INA219Sensor::INA219Sensor(uint8_t address, uint8_t sda_pin, uint8_t scl_pin) 
    : _address(address), _sda_pin(sda_pin), _scl_pin(scl_pin), _initialized(false),
      _error_count(0), _successful_reads(0), _current_lsb(INA219_CURRENT_LSB_DEFAULT),
      _power_lsb(0), _calibration_value(0), _shunt_resistor(0.1f), _max_current(3.2f) {
    
    // Initialize raw data structure
    memset(&_raw_data, 0, sizeof(_raw_data));
    memset(&_converted_data, 0, sizeof(_converted_data));
}

// Initialize the sensor
bool INA219Sensor::begin(float shunt_resistor, float max_current) {
    Serial.println("Initializing INA219 sensor...");
    
    _shunt_resistor = shunt_resistor;
    _max_current = max_current;
    
    // Test I2C connection
    if (!testConnection()) {
        Serial.println("ERROR: INA219 I2C connection failed");
        return false;
    }
    
    // Reset the sensor
    if (!writeRegister(INA219_REG_CONFIG, INA219_CONFIG_RESET)) {
        Serial.println("ERROR: Failed to reset INA219");
        return false;
    }
    delay(10); // Wait for reset to complete
    
    // Calculate calibration values
    calculateCalibration();
    
    // Set calibration register
    setCalibrationRegister();
    
    // Configure the sensor
    uint16_t config = INA219_CONFIG_DEFAULT;
    if (!writeRegister(INA219_REG_CONFIG, config)) {
        Serial.println("ERROR: Failed to configure INA219");
        return false;
    }
    
    delay(10); // Wait for configuration to take effect
    
    // Test reading
    if (!readBusVoltage()) {
        Serial.println("ERROR: INA219 initial read test failed");
        return false;
    }
    
    _initialized = true;
    Serial.printf("INA219 initialized successfully (Voltage monitoring only)\n");
    
    return true;
}

// Read sensor data and populate CAN message structure
bool INA219Sensor::readSensorData(INA219Data& data) {
    if (!_initialized) {
        return false;
    }
    
    // Read only bus voltage (current and power not needed)
    if (!readBusVoltage()) {
        _error_count++;
        return false;
    }
    
    // Convert to raw format for CAN transmission
    data.voltage_raw = voltageToRaw(_converted_data.bus_voltage);
    data.sensor_id = INA219_SENSOR_ID;
    data.status_flags = 0x01; // Only voltage valid
    
    _successful_reads++;
    return true;
}

// Read bus voltage
bool INA219Sensor::readBusVoltage() {
    uint16_t raw_value = readRegister(INA219_REG_BUS_VOLTAGE);
    if (raw_value == 0xFFFF) {
        return false; // Error reading register
    }
    
    _raw_data.bus_voltage_raw = raw_value;
    
    // Convert to voltage (shift right 3 bits, multiply by 4mV LSB)
    _converted_data.bus_voltage = (raw_value >> 3) * INA219_BUS_VOLTAGE_LSB / 1000.0f;
    
    return true;
}

// Read shunt voltage
bool INA219Sensor::readShuntVoltage() {
    int16_t raw_value = (int16_t)readRegister(INA219_REG_SHUNT_VOLTAGE);
    if (raw_value == 0x7FFF) {
        return false; // Error reading register
    }
    
    _raw_data.shunt_voltage_raw = raw_value;
    
    // Convert to voltage (multiply by 10µV LSB)
    _converted_data.shunt_voltage = raw_value * INA219_SHUNT_VOLTAGE_LSB / 1000000.0f;
    
    return true;
}

// Read current
bool INA219Sensor::readCurrent() {
    int16_t raw_value = (int16_t)readRegister(INA219_REG_CURRENT);
    if (raw_value == 0x7FFF) {
        return false; // Error reading register
    }
    
    _raw_data.current_raw = raw_value;
    
    // Convert to current (multiply by current LSB)
    _converted_data.current = raw_value * _current_lsb;
    
    return true;
}

// Read power
bool INA219Sensor::readPower() {
    uint16_t raw_value = readRegister(INA219_REG_POWER);
    if (raw_value == 0xFFFF) {
        return false; // Error reading register
    }
    
    _raw_data.power_raw = raw_value;
    
    // Convert to power (multiply by power LSB)
    _converted_data.power = raw_value * _power_lsb;
    
    return true;
}

// Set calibration values
void INA219Sensor::setCalibration(float shunt_resistor, float max_current) {
    _shunt_resistor = shunt_resistor;
    _max_current = max_current;
    calculateCalibration();
    setCalibrationRegister();
}

// Set bus voltage range
void INA219Sensor::setBusVoltageRange(bool range_16v) {
    uint16_t config = readRegister(INA219_REG_CONFIG);
    if (range_16v) {
        config |= INA219_CONFIG_BVOLTAGERANGE;
    } else {
        config &= ~INA219_CONFIG_BVOLTAGERANGE;
    }
    writeRegister(INA219_REG_CONFIG, config);
}

// Set gain
void INA219Sensor::setGain(uint8_t gain) {
    uint16_t config = readRegister(INA219_REG_CONFIG);
    config &= ~0x1800; // Clear gain bits
    config |= (gain & 0x03) << 11; // Set new gain
    writeRegister(INA219_REG_CONFIG, config);
}

// Set mode
void INA219Sensor::setMode(uint8_t mode) {
    uint16_t config = readRegister(INA219_REG_CONFIG);
    config &= ~0x0007; // Clear mode bits
    config |= mode & 0x07; // Set new mode
    writeRegister(INA219_REG_CONFIG, config);
}

// Print status information
void INA219Sensor::printStatus() const {
    Serial.println("=== INA219 Status ===");
    Serial.printf("Initialized: %s\n", _initialized ? "Yes" : "No");
    Serial.printf("Address: 0x%02X\n", _address);
    Serial.printf("Shunt Resistor: %.3fΩ\n", _shunt_resistor);
    Serial.printf("Max Current: %.1fA\n", _max_current);
    Serial.printf("Current LSB: %.6fA\n", _current_lsb);
    Serial.printf("Power LSB: %.6fW\n", _power_lsb);
    Serial.printf("Calibration: 0x%04X\n", _calibration_value);
    Serial.printf("Successful Reads: %d\n", _successful_reads);
    Serial.printf("Error Count: %d\n", _error_count);
    
    if (_initialized) {
        Serial.printf("Bus Voltage: %.3fV\n", _converted_data.bus_voltage);
    }
    Serial.println("====================");
}

// Test I2C connection
bool INA219Sensor::testConnection() {
    Wire.beginTransmission(_address);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

// Write register
bool INA219Sensor::writeRegister(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.write((value >> 8) & 0xFF); // High byte
    Wire.write(value & 0xFF);        // Low byte
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

// Read register
uint16_t INA219Sensor::readRegister(uint8_t reg) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    uint8_t error = Wire.endTransmission();
    
    if (error != 0) {
        return 0xFFFF; // Error indicator
    }
    
    Wire.requestFrom(_address, (uint8_t)2);
    if (Wire.available() < 2) {
        return 0xFFFF; // Error indicator
    }
    
    uint16_t value = (Wire.read() << 8) | Wire.read();
    return value;
}

// Calculate calibration values
void INA219Sensor::calculateCalibration() {
    // Calculate current LSB for maximum precision
    _current_lsb = _max_current / 32768.0f;
    
    // Calculate power LSB
    _power_lsb = _current_lsb * INA219_POWER_LSB_MULTIPLIER;
    
    // Calculate calibration register value
    _calibration_value = (uint16_t)(0.04096f / (_current_lsb * _shunt_resistor));
}

// Set calibration register
void INA219Sensor::setCalibrationRegister() {
    writeRegister(INA219_REG_CALIBRATION, _calibration_value);
}

// Convert voltage to raw format
uint16_t INA219Sensor::voltageToRaw(float voltage) {
    return (uint16_t)(voltage * 10000.0f); // Convert to 0.1mV units
}

// Convert current to raw format
uint16_t INA219Sensor::currentToRaw(float current) {
    return (uint16_t)(current * 10000.0f); // Convert to 0.1mA units
}

// Convert power to raw format
uint16_t INA219Sensor::powerToRaw(float power) {
    return (uint16_t)(power * 10000.0f); // Convert to 0.1mW units
}
