/*
 * TMP36 CAN Sensor Client
 * ========================
 * 
 * PURPOSE:
 * Sensor client that reads TMP36 temperature data using the tmp36_sensor.h
 * library and sends it via CAN-Bus to the main display unit.
 * 
 * ENVIRONMENT:
 * - Hardware: ESP32 with TMP36 sensor + MCP2515 CAN controller
 * - Platform: PlatformIO with Arduino framework
 * - Libraries: SPI, Adafruit_MCP2515, tmp36_sensor.h
 * 
 * FUNCTIONALITY:
 * 1. Initializes TMP36 sensor using the sensor library
 * 2. Initializes MCP2515 CAN controller for communication
 * 3. Reads temperature data with ADC conversion and calibration
 * 4. Sends temperature data via CAN-Bus to main display unit every 5 seconds
 * 5. Provides comprehensive serial debug output
 * 6. Includes sensor status and heartbeat messages
 * 
 * CONNECTIONS:
 * - TMP36 VCC → ESP32 3.3V
 * - TMP36 GND → ESP32 GND
 * - TMP36 VOUT → ESP32 GPIO 32 (ADC1_CH0)
 * - MCP2515 CS → ESP32 GPIO 5
 * - MCP2515 INT → ESP32 GPIO 2
 * - MCP2515 CLK → ESP32 GPIO 18
 * - MCP2515 MOSI → ESP32 GPIO 23
 * - MCP2515 MISO → ESP32 GPIO 19
 * 
 * CAN-BUS:
 * - Baud Rate: 500 kbps
 * - Message ID: 0x102 (TMP36 temperature)
 * - Sends temperature data with status flags
 * 
 * USAGE:
 * Upload to ESP32 with TMP36 sensor and MCP2515 CAN controller.
 * Uses the tmp36_sensor.h library for reliable sensor communication
 * and sends temperature data via CAN-Bus to the main display unit.
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MCP2515.h>
#include "can_messages.h"
#include "tmp36_sensor.h"

// CAN-Bus Configuration
Adafruit_MCP2515 can;

// Timing variables
static unsigned long last_send = 0;
static const unsigned long SEND_INTERVAL = 5000; // Send every 5 seconds
static unsigned long device_uptime = 0;
static uint16_t error_count = 0;

// Sensor configuration
static uint8_t sensor_id = 2;  // Unique identifier for this sensor
static uint8_t sensor_type = 2; // 2 = TMP36

// Function prototypes
void setupCAN();
void sendTemperatureData(float temperature);
void sendSensorStatus();
void sendHeartbeat();

void setupCAN() {
    Serial.println("🚌 Setting up CAN-Bus...");
    
    // Initialize SPI for MCP2515
    SPI.begin(CAN_CLK_PIN, CAN_MISO_PIN, CAN_MOSI_PIN, CAN_CS_PIN);
    
    // Initialize MCP2515 CAN controller
    if (!can.begin(CAN_500KBPS)) {
        Serial.println("❌ Failed to initialize CAN controller!");
        Serial.println("   Check MCP2515 connections and SPI pins");
        Serial.printf("   CS: GPIO%d, INT: GPIO%d, CLK: GPIO%d, MOSI: GPIO%d, MISO: GPIO%d\n",
                     CAN_CS_PIN, CAN_INT_PIN, CAN_CLK_PIN, CAN_MOSI_PIN, CAN_MISO_PIN);
        error_count++;
        return;
    }
    
    Serial.println("✅ CAN-Bus initialized successfully!");
    Serial.printf("🚌 CAN Speed: 500 kbps\n");
    Serial.printf("🚌 CAN Pins - CS: GPIO%d, INT: GPIO%d, CLK: GPIO%d, MOSI: GPIO%d, MISO: GPIO%d\n",
                 CAN_CS_PIN, CAN_INT_PIN, CAN_CLK_PIN, CAN_MOSI_PIN, CAN_MISO_PIN);
    Serial.printf("🚌 Sensor ID: %d\n", sensor_id);
    Serial.println("🚌 Ready to send temperature data...");
}

void sendTemperatureData(float temperature) {
    // Create TMP36 data message
    TMP36Data data;
    data.temperature_raw = temperatureToRaw(temperature);
    data.sensor_id = sensor_id;
    data.status_flags = 0x01; // Temperature valid flag
    data.reserved = 0; // Reserved for future use
    
    // Send CAN message
    can_frame frame;
    frame.can_id = CAN_MSG_TMP36_TEMPERATURE;
    frame.can_dlc = sizeof(TMP36Data);
    memcpy(frame.data, &data, sizeof(TMP36Data));
    
    if (can.sendMessage(&frame) == MCP2515::ERROR_OK) {
        Serial.printf("📡 CAN TMP36 temperature sent - Temp: %.2f°C (%.1f°F), Sensor ID: %d\n",
                     temperature, (temperature * 9.0/5.0) + 32.0, sensor_id);
    } else {
        Serial.println("❌ Failed to send CAN TMP36 temperature!");
        error_count++;
    }
}

void sendSensorStatus() {
    // Create sensor status message
    SensorStatus status;
    status.sensor_type = sensor_type;
    status.sensor_id = sensor_id;
    status.health_status = (error_count > 10) ? 2 : (error_count > 5) ? 1 : 0; // 0=OK, 1=Warning, 2=Error
    status.battery_level = 100; // Assuming powered by USB/3.3V
    status.uptime_seconds = (uint16_t)(millis() / 1000);
    status.error_count = error_count;
    
    // Send CAN message
    can_frame frame;
    frame.can_id = CAN_MSG_SENSOR_STATUS;
    frame.can_dlc = sizeof(SensorStatus);
    memcpy(frame.data, &status, sizeof(SensorStatus));
    
    if (can.sendMessage(&frame) == MCP2515::ERROR_OK) {
        Serial.printf("📊 Sensor status sent - Health: %d, Errors: %d, Uptime: %ds\n",
                     status.health_status, error_count, status.uptime_seconds);
    } else {
        Serial.println("❌ Failed to send sensor status!");
        error_count++;
    }
}

void sendHeartbeat() {
    // Create heartbeat message
    Heartbeat hb;
    hb.device_type = 3; // 3 = TMP36 sensor
    hb.device_id = sensor_id;
    hb.uptime_seconds = (uint16_t)(millis() / 1000);
    hb.status_flags = 0x00; // Normal status
    hb.free_memory = ESP.getFreeHeap();
    hb.temperature = (uint8_t)getCurrentTemperature(); // Use sensor temperature as device temp
    
    // Send CAN message
    can_frame frame;
    frame.can_id = CAN_MSG_HEARTBEAT;
    frame.can_dlc = sizeof(Heartbeat);
    memcpy(frame.data, &hb, sizeof(Heartbeat));
    
    if (can.sendMessage(&frame) == MCP2515::ERROR_OK) {
        Serial.printf("💓 Heartbeat sent - Uptime: %ds, Memory: %d bytes\n",
                     hb.uptime_seconds, hb.free_memory);
    } else {
        Serial.println("❌ Failed to send heartbeat!");
        error_count++;
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println("🌡️ TMP36 CAN Sensor Client");
    Serial.println("============================");
    
    // Initialize the TMP36 sensor
    initTMP36();
    
    // Print sensor information
    printSensorInfo();
    
    // Setup CAN-Bus connection
    setupCAN();
    
    Serial.println("🚀 Starting temperature monitoring and sending via CAN-Bus...");
    Serial.println();
}

void loop() {
    // Update temperature reading (handles timing internally)
    updateTemperature();
    
    // Send temperature data every 5 seconds
    if (millis() - last_send >= SEND_INTERVAL) {
        float current_temp = getCurrentTemperature();
        
        // Validate sensor reading
        if (isTemperatureSafe()) {
            Serial.printf("📤 Sending CAN temperature: %.2f°C (%.1f°F)\n", 
                         current_temp, (current_temp * 9.0/5.0) + 32.0);
            sendTemperatureData(current_temp);
            
            // Send sensor status every 5th message
            if ((millis() / SEND_INTERVAL) % 5 == 0) {
                sendSensorStatus();
            }
        } else {
            Serial.println("⚠️  Invalid temperature reading, skipping CAN transmission");
            error_count++;
        }
        
        last_send = millis();
    }
    
    // Send heartbeat every 30 seconds
    static unsigned long last_heartbeat = 0;
    if (millis() - last_heartbeat >= 30000) {
        sendHeartbeat();
        last_heartbeat = millis();
    }
    
    // Update device uptime
    device_uptime = millis() / 1000;
    
    // Small delay to prevent overwhelming the system
    delay(100);
}
