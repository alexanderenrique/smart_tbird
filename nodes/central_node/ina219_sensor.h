/*
 * INA219 High-Side Current/Power Monitor Sensor Class
 * ==================================================
 * 
 * This class provides a high-level interface for the INA219 I2C current/power
 * monitor sensor, designed for battery voltage and current monitoring in
 * automotive applications.
 * 
 * Features:
 * - I2C communication with INA219
 * - Bus voltage measurement (0-26V)
 * - Current measurement via shunt resistor
 * - Power calculation (voltage × current)
 * - Configurable shunt resistor and current range
 * - Error handling and sensor health monitoring
 * - Automotive-optimized for 12V battery systems
 */

#ifndef INA219_SENSOR_H
#define INA219_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include "../shared/can_lib/can_messages.h"

// INA219 Register Addresses
#define INA219_REG_CONFIG           0x00
#define INA219_REG_SHUNT_VOLTAGE    0x01
#define INA219_REG_BUS_VOLTAGE      0x02
#define INA219_REG_POWER            0x03
#define INA219_REG_CURRENT          0x04
#define INA219_REG_CALIBRATION      0x05

// INA219 Configuration Register Bits
#define INA219_CONFIG_RESET         0x8000
#define INA219_CONFIG_BVOLTAGERANGE 0x2000
#define INA219_CONFIG_GAIN_1_40MV   0x0000
#define INA219_CONFIG_GAIN_2_80MV   0x0800
#define INA219_CONFIG_GAIN_4_160MV  0x1000
#define INA219_CONFIG_GAIN_8_320MV  0x1800
#define INA219_CONFIG_BADCRES_12BIT 0x0400
#define INA219_CONFIG_SADCRES_12BIT_1S_532US 0x0018
#define INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS 0x0007

// INA219 Default Configuration
#define INA219_CONFIG_DEFAULT       (INA219_CONFIG_BVOLTAGERANGE_16V | \
                                     INA219_CONFIG_GAIN_2_80MV | \
                                     INA219_CONFIG_BADCRES_12BIT | \
                                     INA219_CONFIG_SADCRES_12BIT_1S_532US | \
                                     INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS)

// INA219 Calibration and Scaling
#define INA219_BUS_VOLTAGE_LSB      4.0f      // 4mV per LSB
#define INA219_SHUNT_VOLTAGE_LSB    10.0f     // 10µV per LSB
#define INA219_CURRENT_LSB_DEFAULT  0.1f      // 0.1mA per LSB (default)
#define INA219_POWER_LSB_MULTIPLIER 20.0f     // Power LSB = Current LSB × 20

class INA219Sensor {
private:
    uint8_t _address;
    uint8_t _sda_pin;
    uint8_t _scl_pin;
    bool _initialized;
    uint16_t _error_count;
    uint16_t _successful_reads;
    
    // Calibration values
    float _current_lsb;
    float _power_lsb;
    uint16_t _calibration_value;
    float _shunt_resistor;
    float _max_current;
    
    // Raw sensor data
    struct {
        uint16_t bus_voltage_raw;
        int16_t shunt_voltage_raw;
        int16_t current_raw;
        uint16_t power_raw;
    } _raw_data;
    
    // Converted values
    struct {
        float bus_voltage;      // Volts
        float shunt_voltage;    // Volts
        float current;          // Amperes
        float power;            // Watts
    } _converted_data;

public:
    // Constructor
    INA219Sensor(uint8_t address = 0x40, uint8_t sda_pin = 21, uint8_t scl_pin = 22);
    
    // Initialization
    bool begin(float shunt_resistor = 0.1f, float max_current = 3.2f);
    bool isInitialized() const { return _initialized; }
    
    // Data reading
    bool readSensorData(INA219Data& data);
    bool readBusVoltage();
    bool readShuntVoltage();
    bool readCurrent();
    bool readPower();
    
    // Configuration
    void setCalibration(float shunt_resistor, float max_current);
    void setBusVoltageRange(bool range_16v = true);
    void setGain(uint8_t gain);
    void setMode(uint8_t mode);
    
    // Utility functions
    float getBusVoltage() const { return _converted_data.bus_voltage; }
    float getShuntVoltage() const { return _converted_data.shunt_voltage; }
    float getCurrent() const { return _converted_data.current; }
    float getPower() const { return _converted_data.power; }
    
    // Status and diagnostics
    uint16_t getErrorCount() const { return _error_count; }
    uint16_t getSuccessfulReads() const { return _successful_reads; }
    void printStatus() const;
    
private:
    // Low-level I2C functions
    bool writeRegister(uint8_t reg, uint16_t value);
    uint16_t readRegister(uint8_t reg);
    bool testConnection();
    
    // Calibration functions
    void calculateCalibration();
    void setCalibrationRegister();
    
    // Data conversion
    void convertRawData();
    uint16_t voltageToRaw(float voltage);
    uint16_t currentToRaw(float current);
    uint16_t powerToRaw(float power);
};

#endif // INA219_SENSOR_H
