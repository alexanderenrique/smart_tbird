/*
 * MPU6050 6-Axis IMU Sensor Class
 * ===============================
 * 
 * This class provides a high-level interface for the MPU6050 6-axis IMU sensor,
 * including acceleration and gyroscope data reading, and maximum value tracking
 * for automotive G-force monitoring.
 * 
 * Features:
 * - I2C communication with MPU6050
 * - Acceleration and gyroscope data reading
 * - Maximum value tracking for each axis
 * - Automotive-optimized configuration (±2g, ±250°/s)
 * - Automatic max value reset functionality
 * - Error handling and sensor health monitoring
 */

#ifndef MPU6050_SENSOR_H
#define MPU6050_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "../shared/can_lib/can_messages.h"

// MPU6050 Register Addresses
#define MPU6050_ADDR                0x68
#define MPU6050_WHO_AM_I            0x75
#define MPU6050_PWR_MGMT_1          0x6B
#define MPU6050_PWR_MGMT_2          0x6C
#define MPU6050_ACCEL_XOUT_H        0x3B
#define MPU6050_ACCEL_XOUT_L        0x3C
#define MPU6050_ACCEL_YOUT_H        0x3D
#define MPU6050_ACCEL_YOUT_L        0x3E
#define MPU6050_ACCEL_ZOUT_H        0x3F
#define MPU6050_ACCEL_ZOUT_L        0x40
#define MPU6050_TEMP_OUT_H          0x41
#define MPU6050_TEMP_OUT_L          0x42
#define MPU6050_GYRO_XOUT_H         0x43
#define MPU6050_GYRO_XOUT_L         0x44
#define MPU6050_GYRO_YOUT_H         0x45
#define MPU6050_GYRO_YOUT_L         0x46
#define MPU6050_GYRO_ZOUT_H         0x47
#define MPU6050_GYRO_ZOUT_L         0x48
#define MPU6050_ACCEL_CONFIG        0x1C
#define MPU6050_GYRO_CONFIG         0x1B
#define MPU6050_CONFIG              0x1A

// MPU6050 Configuration Values
#define MPU6050_ACCEL_RANGE_2G      0x00    // ±2g
#define MPU6050_ACCEL_RANGE_4G      0x08    // ±4g
#define MPU6050_ACCEL_RANGE_8G      0x10    // ±8g
#define MPU6050_ACCEL_RANGE_16G     0x18    // ±16g

#define MPU6050_GYRO_RANGE_250      0x00    // ±250°/s
#define MPU6050_GYRO_RANGE_500      0x08    // ±500°/s
#define MPU6050_GYRO_RANGE_1000     0x10    // ±1000°/s
#define MPU6050_GYRO_RANGE_2000     0x18    // ±2000°/s

// DLPF (Digital Low-Pass Filter) Configuration
#define MPU6050_DLPF_CFG_0          0x00    // 260Hz accel, 256Hz gyro
#define MPU6050_DLPF_CFG_1          0x01    // 184Hz accel, 188Hz gyro
#define MPU6050_DLPF_CFG_2          0x02    // 94Hz accel, 98Hz gyro
#define MPU6050_DLPF_CFG_3          0x03    // 44Hz accel, 42Hz gyro
#define MPU6050_DLPF_CFG_4          0x04    // 21Hz accel, 20Hz gyro
#define MPU6050_DLPF_CFG_5          0x05    // 10Hz accel, 10Hz gyro
#define MPU6050_DLPF_CFG_6          0x06    // 5Hz accel, 5Hz gyro
#define MPU6050_DLPF_CFG_7          0x07    // Reserved

// Sensitivity values for ±2g and ±250°/s
#define MPU6050_ACCEL_SENSITIVITY   16384.0f    // LSB/g for ±2g
#define MPU6050_GYRO_SENSITIVITY    131.0f      // LSB/°/s for ±250°/s

class MPU6050Sensor {
private:
    uint8_t _address;
    uint8_t _sda_pin;
    uint8_t _scl_pin;
    bool _initialized;
    uint16_t _error_count;
    uint16_t _successful_reads;
    
    // Maximum value tracking
    int16_t _max_accel_x;
    int16_t _max_accel_y;
    int16_t _max_accel_z;
    int16_t _max_gyro_x;
    int16_t _max_gyro_y;
    int16_t _max_gyro_z;
    
    // Max value reset tracking
    unsigned long _last_max_reset;
    uint16_t _max_reset_counter;
    
    // Smoothing filter variables
    static const uint8_t SMOOTH_BUFFER_SIZE = 50; // 50 samples = 0.5 seconds at 100Hz
    int16_t _smooth_buffer_x[SMOOTH_BUFFER_SIZE];
    int16_t _smooth_buffer_y[SMOOTH_BUFFER_SIZE];
    int16_t _smooth_buffer_z[SMOOTH_BUFFER_SIZE];
    uint8_t _smooth_index;
    uint16_t _smooth_sample_count;
    
    // Raw sensor data
    struct {
        int16_t accel_x, accel_y, accel_z;
        int16_t gyro_x, gyro_y, gyro_z;
        int16_t temperature;
    } _raw_data;

public:
    // Constructor
    MPU6050Sensor(uint8_t address = MPU6050_ADDR, uint8_t sda_pin = 21, uint8_t scl_pin = 22);
    
    // Initialization
    bool begin();
    bool isInitialized() const { return _initialized; }
    
    // Data reading
    bool readSensorData(MPU6050Data& data);
    bool readMaxValues(MPU6050MaxData& max_data);
    bool readSmoothedData(MPU6050SmoothedData& smoothed_data);
    
    // Max value management
    void resetMaxValues();
    void updateMaxValues();
    
    // Utility functions
    float getAccelerationX() const { return _raw_data.accel_x / MPU6050_ACCEL_SENSITIVITY; }
    float getAccelerationY() const { return _raw_data.accel_y / MPU6050_ACCEL_SENSITIVITY; }
    float getAccelerationZ() const { return _raw_data.accel_z / MPU6050_ACCEL_SENSITIVITY; }
    float getGyroX() const { return _raw_data.gyro_x / MPU6050_GYRO_SENSITIVITY; }
    float getGyroY() const { return _raw_data.gyro_y / MPU6050_GYRO_SENSITIVITY; }
    float getGyroZ() const { return _raw_data.gyro_z / MPU6050_GYRO_SENSITIVITY; }
    float getTemperature() const { return _raw_data.temperature / 340.0f + 36.53f; }
    
    // Status and diagnostics
    uint16_t getErrorCount() const { return _error_count; }
    uint16_t getSuccessfulReads() const { return _successful_reads; }
    void printStatus() const;
    
private:
    // Low-level I2C functions
    bool writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    bool readRegisters(uint8_t reg, uint8_t* buffer, uint8_t length);
    
    // Sensor configuration
    bool configureSensor();
    bool testConnection();
    
    // Data processing
    void processRawData();
    void updateMaxTracking();
    void updateSmoothingFilter();
};

#endif // MPU6050_SENSOR_H
