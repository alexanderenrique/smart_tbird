/*
 * CAN Message Definitions for Smart Thunderbird Project
 * ===================================================
 * 
 * PURPOSE:
 * Defines all CAN message IDs, data structures, and constants for communication
 * between the main display unit and sensor devices.
 * 
 * CAN BUS SPECIFICATIONS:
 * - Baud Rate: 500 kbps (standard automotive)
 * - Message Format: Standard CAN 2.0A (11-bit IDs)
 * - Hardware: MCP2515 CAN controller with TJA1050 transceiver
 * 
 * MESSAGE ID ASSIGNMENT:
 * - 0x100-0x1FF: Sensor data messages
 * - 0x200-0x2FF: System control messages
 * - 0x300-0x3FF: Status and diagnostic messages
 * 
 * USAGE:
 * Include this header in all CAN-enabled devices to ensure consistent
 * message definitions across the system.
 */

#ifndef CAN_MESSAGES_H
#define CAN_MESSAGES_H

#include <Arduino.h>

// ============================================================================
// CAN MESSAGE IDs
// ============================================================================

// Sensor Data Messages (0x100-0x1FF)
#define CAN_MSG_SHT31_TEMP_HUMIDITY    0x101    // SHT31 temperature and humidity
#define CAN_MSG_TMP36_TEMPERATURE      0x102    // TMP36 temperature only
#define CAN_MSG_MPU6050_IMU            0x103    // MPU6050 6-axis IMU data
#define CAN_MSG_SENSOR_STATUS          0x104    // Sensor health and status

// System Control Messages (0x200-0x2FF)
#define CAN_MSG_DISPLAY_REQUEST        0x201    // Request sensor data from display
#define CAN_MSG_SYSTEM_RESET           0x202    // System reset command
#define CAN_MSG_POWER_MODE             0x203    // Power mode control

// Status and Diagnostic Messages (0x300-0x3FF)
#define CAN_MSG_HEARTBEAT             0x301    // Device heartbeat/status
#define CAN_MSG_ERROR_REPORT          0x302    // Error reporting
#define CAN_MSG_DEBUG_INFO            0x303    // Debug information

// ============================================================================
// MESSAGE DATA STRUCTURES
// ============================================================================

// SHT31 Temperature and Humidity Data (8 bytes)
struct SHT31Data {
    uint16_t temperature_raw;     // Temperature in 0.01°C units (e.g., 2500 = 25.00°C)
    uint16_t humidity_raw;        // Humidity in 0.1% units (e.g., 650 = 65.0%)
    uint8_t sensor_id;            // Unique sensor identifier (0-255)
    uint8_t status_flags;         // Status flags (bit 0: temp valid, bit 1: humidity valid)
    uint16_t checksum;            // Simple checksum for data validation
} __attribute__((packed));

// TMP36 Temperature Data (8 bytes)
struct TMP36Data {
    uint16_t temperature_raw;     // Temperature in 0.01°C units
    uint8_t sensor_id;            // Unique sensor identifier (0-255)
    uint8_t status_flags;         // Status flags (bit 0: temp valid)
    uint32_t reserved;            // Reserved for future use
} __attribute__((packed));

// MPU6050 IMU Data (8 bytes)
struct MPU6050Data {
    int16_t accel_x;              // X-axis acceleration in raw units
    int16_t accel_y;              // Y-axis acceleration in raw units
    int16_t accel_z;              // Z-axis acceleration in raw units
    uint8_t sensor_id;            // Unique sensor identifier (0-255)
    uint8_t status_flags;         // Status flags (bit 0: accel valid, bit 1: gyro valid)
} __attribute__((packed));

// MPU6050 Max Values Data (8 bytes) - for tracking peak G-forces
struct MPU6050MaxData {
    int16_t max_accel_x;          // Maximum X-axis acceleration recorded
    int16_t max_accel_y;          // Maximum Y-axis acceleration recorded
    int16_t max_accel_z;          // Maximum Z-axis acceleration recorded
    uint8_t sensor_id;            // Unique sensor identifier (0-255)
    uint8_t status_flags;         // Status flags (bit 0: max values valid)
    uint16_t reset_counter;       // Counter for max value resets
} __attribute__((packed));

// Sensor Status Message (8 bytes)
struct SensorStatus {
    uint8_t sensor_type;          // 1=SHT31, 2=TMP36, 3=Other
    uint8_t sensor_id;            // Unique sensor identifier
    uint8_t health_status;        // 0=OK, 1=Warning, 2=Error, 3=Critical
    uint8_t battery_level;        // Battery level 0-100%
    uint16_t uptime_seconds;      // Device uptime in seconds
    uint16_t error_count;         // Number of errors since last reset
} __attribute__((packed));

// Heartbeat Message (8 bytes)
struct Heartbeat {
    uint8_t device_type;          // 1=Display, 2=SHT31, 3=TMP36
    uint8_t device_id;            // Unique device identifier
    uint16_t uptime_seconds;      // Device uptime in seconds
    uint8_t status_flags;         // Status flags
    uint16_t free_memory;         // Free memory in bytes
    uint8_t temperature;          // Device internal temperature (°C)
} __attribute__((packed));

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Convert temperature from raw format to float
inline float rawToTemperature(uint16_t raw_temp) {
    return raw_temp / 100.0f;
}

// Convert temperature from float to raw format
inline uint16_t temperatureToRaw(float temp) {
    return (uint16_t)(temp * 100.0f);
}

// Convert humidity from raw format to float
inline float rawToHumidity(uint16_t raw_humidity) {
    return raw_humidity / 10.0f;
}

// Convert humidity from float to raw format
inline uint16_t humidityToRaw(float humidity) {
    return (uint16_t)(humidity * 10.0f);
}

// Convert MPU6050 raw acceleration to G-forces
inline float rawToAcceleration(int16_t raw_accel) {
    // MPU6050 default sensitivity is ±2g = ±16384 LSB/g
    return raw_accel / 16384.0f;
}

// Convert G-forces to MPU6050 raw acceleration
inline int16_t accelerationToRaw(float g_force) {
    return (int16_t)(g_force * 16384.0f);
}

// Calculate simple checksum for data validation
inline uint16_t calculateChecksum(const uint8_t* data, size_t length) {
    uint16_t checksum = 0;
    for (size_t i = 0; i < length - 2; i++) {  // Exclude checksum field
        checksum += data[i];
    }
    return checksum;
}

// Validate checksum
inline bool validateChecksum(const uint8_t* data, size_t length) {
    uint16_t calculated = calculateChecksum(data, length - 2);
    uint16_t received = (data[length - 2] << 8) | data[length - 1];
    return calculated == received;
}

// ============================================================================
// CAN BUS CONFIGURATION
// ============================================================================

// CAN Bus timing for 500 kbps
#define CAN_500KBPS_TIMING MCP_16MHZ_500KBPS

// CAN Bus pins (can be customized per device)
#define CAN_CS_PIN      5    // Chip select pin for MCP2515
#define CAN_INT_PIN     2    // Interrupt pin for MCP2515
#define CAN_CLK_PIN     18   // SPI clock pin
#define CAN_MOSI_PIN    23   // SPI MOSI pin
#define CAN_MISO_PIN    19   // SPI MISO pin

// CAN Message priorities (lower number = higher priority)
#define PRIORITY_HIGH     1    // Critical messages (errors, system reset)
#define PRIORITY_NORMAL   2    // Regular sensor data
#define PRIORITY_LOW      3    // Heartbeat, debug info

// ============================================================================
// ERROR CODES
// ============================================================================

#define ERROR_NONE                0x00
#define ERROR_SENSOR_READ_FAILED  0x01
#define ERROR_CAN_BUS_ERROR       0x02
#define ERROR_MEMORY_LOW          0x03
#define ERROR_BATTERY_LOW         0x04
#define ERROR_TEMPERATURE_HIGH    0x05
#define ERROR_HUMIDITY_HIGH       0x06

#endif // CAN_MESSAGES_H
