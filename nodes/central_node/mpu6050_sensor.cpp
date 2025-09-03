/*
 * MPU6050 6-Axis IMU Sensor Class Implementation
 * =============================================
 */

#include "mpu6050_sensor.h"

// Constructor
MPU6050Sensor::MPU6050Sensor(uint8_t address, uint8_t sda_pin, uint8_t scl_pin) 
    : _address(address), _sda_pin(sda_pin), _scl_pin(scl_pin), _initialized(false),
      _error_count(0), _successful_reads(0), _max_accel_x(0), _max_accel_y(0), _max_accel_z(0),
      _max_gyro_x(0), _max_gyro_y(0), _max_gyro_z(0), _last_max_reset(0), _max_reset_counter(0) {
    
    // Initialize raw data structure
    memset(&_raw_data, 0, sizeof(_raw_data));
}

// Initialize the sensor
bool MPU6050Sensor::begin() {
    Serial.println("Initializing MPU6050 sensor...");
    
    // Initialize I2C if not already done
    Wire.begin(_sda_pin, _scl_pin);
    Wire.setClock(400000); // 400kHz I2C speed
    
    // Test connection
    if (!testConnection()) {
        Serial.println("ERROR: MPU6050 connection test failed");
        _error_count++;
        return false;
    }
    
    // Configure sensor
    if (!configureSensor()) {
        Serial.println("ERROR: MPU6050 configuration failed");
        _error_count++;
        return false;
    }
    
    // Initialize max value tracking
    resetMaxValues();
    
    _initialized = true;
    Serial.println("MPU6050 initialized successfully");
    return true;
}

// Test I2C connection to MPU6050
bool MPU6050Sensor::testConnection() {
    uint8_t who_am_i = readRegister(MPU6050_WHO_AM_I);
    if (who_am_i != 0x68) {
        Serial.printf("ERROR: MPU6050 WHO_AM_I = 0x%02X (expected 0x68)\n", who_am_i);
        return false;
    }
    return true;
}

// Configure sensor for automotive use
bool MPU6050Sensor::configureSensor() {
    // Wake up the sensor (clear sleep bit)
    if (!writeRegister(MPU6050_PWR_MGMT_1, 0x00)) {
        return false;
    }
    delay(100);
    
    // Configure DLPF (Digital Low-Pass Filter) for 100Hz sampling
    // DLPF_CFG_3 = ~44Hz cutoff frequency (good for 100Hz sampling)
    if (!writeRegister(MPU6050_CONFIG, MPU6050_DLPF_CFG_3)) {
        return false;
    }
    
    // Configure accelerometer range (±2g for automotive use)
    if (!writeRegister(MPU6050_ACCEL_CONFIG, MPU6050_ACCEL_RANGE_2G)) {
        return false;
    }
    
    // Configure gyroscope range (±250°/s)
    if (!writeRegister(MPU6050_GYRO_CONFIG, MPU6050_GYRO_RANGE_250)) {
        return false;
    }
    
    // Disable all sensors in PWR_MGMT_2 (we only need accel and gyro)
    if (!writeRegister(MPU6050_PWR_MGMT_2, 0x00)) {
        return false;
    }
    
    delay(100);
    return true;
}

// Read sensor data and populate MPU6050Data structure
bool MPU6050Sensor::readSensorData(MPU6050Data& data) {
    if (!_initialized) {
        return false;
    }
    
    // Read all sensor data in one I2C transaction
    uint8_t buffer[14];
    if (!readRegisters(MPU6050_ACCEL_XOUT_H, buffer, 14)) {
        Serial.println("ERROR: Failed to read MPU6050 sensor data");
        _error_count++;
        return false;
    }
    
    // Process raw data
    _raw_data.accel_x = (int16_t)((buffer[0] << 8) | buffer[1]);
    _raw_data.accel_y = (int16_t)((buffer[2] << 8) | buffer[3]);
    _raw_data.accel_z = (int16_t)((buffer[4] << 8) | buffer[5]);
    _raw_data.temperature = (int16_t)((buffer[6] << 8) | buffer[7]);
    _raw_data.gyro_x = (int16_t)((buffer[8] << 8) | buffer[9]);
    _raw_data.gyro_y = (int16_t)((buffer[10] << 8) | buffer[11]);
    _raw_data.gyro_z = (int16_t)((buffer[12] << 8) | buffer[13]);
    
    // Update max value tracking
    updateMaxTracking();
    
    // Populate data structure
    data.accel_x = _raw_data.accel_x;
    data.accel_y = _raw_data.accel_y;
    data.accel_z = _raw_data.accel_z;
    data.sensor_id = MPU6050_SENSOR_ID;
    data.status_flags = 0x03; // Both accel and gyro valid
    
    _successful_reads++;
    return true;
}

// Read maximum values recorded (absolute max since power-on)
bool MPU6050Sensor::readMaxValues(MPU6050MaxData& max_data) {
    if (!_initialized) {
        return false;
    }
    
    // Populate max data structure with absolute maximums since power-on
    max_data.max_accel_x = _max_accel_x;
    max_data.max_accel_y = _max_accel_y;
    max_data.max_accel_z = _max_accel_z;
    max_data.sensor_id = MPU6050_SENSOR_ID;
    max_data.status_flags = 0x01; // Max values valid
    max_data.reset_counter = _max_reset_counter; // Shows number of manual resets
    
    return true;
}

// Reset maximum values (manual reset only - absolute max tracking since power-on)
void MPU6050Sensor::resetMaxValues() {
    _max_accel_x = 0;
    _max_accel_y = 0;
    _max_accel_z = 0;
    _max_gyro_x = 0;
    _max_gyro_y = 0;
    _max_gyro_z = 0;
    _last_max_reset = millis();
    _max_reset_counter++;
    
    Serial.println("MPU6050 max values manually reset - now tracking new absolute maximums");
}

// Update maximum value tracking
void MPU6050Sensor::updateMaxTracking() {
    // Update acceleration max values (absolute values)
    if (abs(_raw_data.accel_x) > abs(_max_accel_x)) {
        _max_accel_x = _raw_data.accel_x;
    }
    if (abs(_raw_data.accel_y) > abs(_max_accel_y)) {
        _max_accel_y = _raw_data.accel_y;
    }
    if (abs(_raw_data.accel_z) > abs(_max_accel_z)) {
        _max_accel_z = _raw_data.accel_z;
    }
    
    // Update gyroscope max values (absolute values)
    if (abs(_raw_data.gyro_x) > abs(_max_gyro_x)) {
        _max_gyro_x = _raw_data.gyro_x;
    }
    if (abs(_raw_data.gyro_y) > abs(_max_gyro_y)) {
        _max_gyro_y = _raw_data.gyro_y;
    }
    if (abs(_raw_data.gyro_z) > abs(_max_gyro_z)) {
        _max_gyro_z = _raw_data.gyro_z;
    }
}

// Print sensor status
void MPU6050Sensor::printStatus() const {
    Serial.println("=== MPU6050 Status ===");
    Serial.printf("Initialized: %s\n", _initialized ? "Yes" : "No");
    Serial.printf("Sample rate: 100Hz (10ms intervals)\n");
    Serial.printf("DLPF: ~44Hz cutoff (CFG_3)\n");
    Serial.printf("Successful reads: %d\n", _successful_reads);
    Serial.printf("Error count: %d\n", _error_count);
    Serial.printf("Manual resets: %d (absolute max since power-on)\n", _max_reset_counter);
    
    if (_initialized) {
        Serial.printf("Current acceleration: X=%.2fg, Y=%.2fg, Z=%.2fg\n", 
                     getAccelerationX(), getAccelerationY(), getAccelerationZ());
        Serial.printf("Current gyroscope: X=%.1f°/s, Y=%.1f°/s, Z=%.1f°/s\n",
                     getGyroX(), getGyroY(), getGyroZ());
        Serial.printf("Temperature: %.1f°C\n", getTemperature());
        Serial.printf("Absolute max acceleration: X=%.2fg, Y=%.2fg, Z=%.2fg\n",
                     _max_accel_x / MPU6050_ACCEL_SENSITIVITY,
                     _max_accel_y / MPU6050_ACCEL_SENSITIVITY,
                     _max_accel_z / MPU6050_ACCEL_SENSITIVITY);
    }
    Serial.println("=====================");
}

// Low-level I2C write function
bool MPU6050Sensor::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.write(value);
    uint8_t result = Wire.endTransmission();
    return (result == 0);
}

// Low-level I2C read function
uint8_t MPU6050Sensor::readRegister(uint8_t reg) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(_address, (uint8_t)1);
    return Wire.read();
}

// Low-level I2C read multiple registers
bool MPU6050Sensor::readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length) {
    Wire.beginTransmission(_address);
    Wire.write(reg);
    uint8_t result = Wire.endTransmission(false);
    if (result != 0) {
        return false;
    }
    
    Wire.requestFrom(_address, length);
    for (uint8_t i = 0; i < length; i++) {
        if (Wire.available()) {
            buffer[i] = Wire.read();
        } else {
            return false;
        }
    }
    return true;
}
