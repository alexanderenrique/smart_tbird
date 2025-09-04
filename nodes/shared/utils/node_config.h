/*
 * Node Configuration - Hardware-specific pin definitions
 * =====================================================
 * 
 * This file defines hardware configurations for different node types
 * and provides a unified interface for accessing pin assignments.
 * 
 * ACTIVE NODES:
 * - DISPLAY_NODE: Main display unit with TFT and LDR
 * - CENTRAL_NODE: Primary sensor hub with SHT31 + MPU6050
 */

#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

#include <Arduino.h>

// ============================================================================
// NODE TYPE DEFINITIONS
// ============================================================================
#define DISPLAY_NODE    1
#define CENTRAL_NODE    2

// ============================================================================
// DISPLAY NODE CONFIGURATION (ESP32 with TFT Display + LDR)
// ============================================================================
#ifdef NODE_TYPE
#if NODE_TYPE == DISPLAY_NODE

// TFT Display Pins
#define TFT_CS_PIN      27
#define TFT_DC_PIN      12
#define TFT_RST_PIN     13
#define TFT_MOSI_PIN    33
#define TFT_SCLK_PIN    32
#define TFT_MISO_PIN    19
#define TFT_BL_PIN      25

// Touch Screen Pins
#define TOUCH_CS_PIN    14
#define TOUCH_IRQ_PIN   13

// LDR Sensor Pins (for auto-dimming)
#define LDR_ANALOG_PIN      36
#define LDR_PULLUP_PIN      39

// CAN Bus Pins
#define CAN_CS_PIN      5
#define CAN_INT_PIN     2
#define CAN_CLK_PIN     18
#define CAN_MOSI_PIN    23
#define CAN_MISO_PIN    19

// Status LED
#define STATUS_LED_PIN  2

// Node Configuration
#define NODE_NAME       "Display Node"
#define SENSOR_READ_INTERVAL  2000    // 2 seconds (faster for LDR)
#define HEARTBEAT_INTERVAL    30000   // 30 seconds

// LDR Configuration (from proven ldr_auto_dim_test)
#define LDR_VOLTAGE_REF       3.3     // Reference voltage
#define LDR_READ_INTERVAL     500     // Read LDR every 500ms (faster response)
#define LDR_BRIGHTNESS_MIN    20      // Minimum brightness (prevents flickering)
#define LDR_BRIGHTNESS_MAX    255     // Maximum brightness
#define LDR_DARK_THRESHOLD    100     // LDR value below which is "dark"
#define LDR_BRIGHT_THRESHOLD  800     // LDR value above which is "bright"
#define LDR_GAMMA             2.2f    // Gamma correction for human perception
#define LDR_SMOOTHING_FACTOR  8       // Higher = slower changes (1-16)
#define LDR_AVERAGE_SAMPLES   100     // Number of samples to average (10 seconds)
#define LDR_PWM_FREQUENCY     25000   // PWM frequency (25kHz for less flicker)
#define LDR_PWM_RESOLUTION    8       // PWM resolution (8-bit)

#endif // DISPLAY_NODE
#endif // NODE_TYPE



// ============================================================================
// CENTRAL NODE CONFIGURATION (ESP32 with SHT31 + MPU6050)
// ============================================================================
#ifdef NODE_TYPE
#if NODE_TYPE == CENTRAL_NODE

// SHT31 Sensor Pins (I2C)
#define SHT31_SDA_PIN   21
#define SHT31_SCL_PIN   22
#define SHT31_ADDRESS   0x44

// MPU6050 Sensor Pins (I2C - same bus as SHT31)
#define MPU6050_SDA_PIN 21
#define MPU6050_SCL_PIN 22
#define MPU6050_ADDRESS 0x68

// INA219 Battery Monitor Pins (I2C - same bus as other sensors)
#define INA219_SDA_PIN   21
#define INA219_SCL_PIN   22
#define INA219_ADDRESS   0x40

// CAN Bus Pins
#define CAN_CS_PIN      5
#define CAN_INT_PIN     2
#define CAN_CLK_PIN     18
#define CAN_MOSI_PIN    23
#define CAN_MISO_PIN    19

// Status LED
#define STATUS_LED_PIN  2

// Node Configuration
#define NODE_NAME       "Central Node"
#define SENSOR_READ_INTERVAL  10      // 10ms = 100Hz for MPU6050
#define SHT31_READ_INTERVAL   2000    // 2 seconds = 0.5Hz for SHT31
#define HEARTBEAT_INTERVAL    30000   // 30 seconds
#define SHT31_SENSOR_ID       1
#define MPU6050_SENSOR_ID     2
#define INA219_SENSOR_ID      3

// MPU6050 Configuration
#define MPU6050_ACCEL_RANGE   2       // ±2g range for automotive use
#define MPU6050_GYRO_RANGE    250     // ±250°/s range
#define MPU6050_DLPF_CONFIG   3       // Digital Low-Pass Filter: ~44Hz cutoff

// INA219 Configuration
#define INA219_READ_INTERVAL  500    // 500 ms (2Hz)
#define INA219_SHUNT_RESISTOR 0.1f    // 0.1 ohm shunt resistor (typical for INA219)
#define INA219_MAX_CURRENT    3.2f    // Maximum expected current (3.2A)
#define INA219_BUS_VOLTAGE    16.0f   // Maximum bus voltage (16V)

#endif // CENTRAL_NODE
#endif // NODE_TYPE

// ============================================================================
// COMMON CONFIGURATION
// ============================================================================

// Default values if not defined
#ifndef NODE_NAME
#define NODE_NAME       "Unknown Node"
#endif

#ifndef SENSOR_READ_INTERVAL
#define SENSOR_READ_INTERVAL  5000
#endif

#ifndef HEARTBEAT_INTERVAL
#define HEARTBEAT_INTERVAL    30000
#endif

#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN  2
#endif

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Get node type name as string
inline const char* getNodeTypeName() {
    #ifdef NODE_TYPE
    switch (NODE_TYPE) {
        case DISPLAY_NODE: return "Display";
        case CENTRAL_NODE: return "Central";
        default:           return "Unknown";
    }
    #else
    return "Undefined";
    #endif
}

// Get node ID as string
inline const char* getNodeIdString() {
    #ifdef NODE_ID
    static char id_str[8];
    snprintf(id_str, sizeof(id_str), "%d", NODE_ID);
    return id_str;
    #else
    return "0";
    #endif
}

// Print node configuration
inline void printNodeConfig() {
    Serial.println("=== Node Configuration ===");
    Serial.printf("Name: %s\n", NODE_NAME);
    Serial.printf("Type: %s (ID: %s)\n", getNodeTypeName(), getNodeIdString());
    Serial.printf("Sensor Read Interval: %d ms\n", SENSOR_READ_INTERVAL);
    #ifdef SHT31_READ_INTERVAL
    Serial.printf("SHT31 Read Interval: %d ms\n", SHT31_READ_INTERVAL);
    #endif
    Serial.printf("Heartbeat Interval: %d ms\n", HEARTBEAT_INTERVAL);
    Serial.printf("Status LED Pin: %d\n", STATUS_LED_PIN);
    Serial.printf("CAN CS Pin: %d\n", CAN_CS_PIN);
    Serial.printf("CAN INT Pin: %d\n", CAN_INT_PIN);
    Serial.println("==========================");
}

#endif // NODE_CONFIG_H
