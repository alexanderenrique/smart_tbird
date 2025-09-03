/*
 * SHT31 CAN Sensor Client
 * ========================
 * 
 * PURPOSE:
 * Sensor client that reads SHT31 temperature and humidity data using the sht31_sensor.h
 * library and sends it via CAN-Bus to the main display unit.
 * 
 * ENVIRONMENT:
 * - Hardware: ESP32-C3 with SHT31 sensor + MCP2515 CAN controller
 * - Platform: PlatformIO with Arduino framework
 * - Libraries: SPI, Adafruit_MCP2515, sht31_sensor.h
 * 
 * FUNCTIONALITY:
 * 1. Initializes SHT31 sensor using the sensor library
 * 2. Initializes MCP2515 CAN controller for communication
 * 3. Reads temperature and humidity data with error handling
 * 4. Sends data via CAN-Bus to main display unit every 5 seconds
 * 5. Uses built-in LED (GPIO 8) to indicate CAN communication status
 * 6. Provides comprehensive serial debug output
 * 7. Includes sensor status and heartbeat messages
 * 
 * CONNECTIONS:
 * - SHT31 VIN → ESP32 3.3V
 * - SHT31 GND → ESP32 GND
 * - SHT31 SDA → ESP32 GPIO 8
 * - SHT31 SCL → ESP32 GPIO 9
 * - MCP2515 CS → ESP32 GPIO 5
 * - MCP2515 INT → ESP32 GPIO 2
 * - MCP2515 CLK → ESP32 GPIO 18
 * - MCP2515 MOSI → ESP32 GPIO 23
 * - MCP2515 MISO → ESP32 GPIO 19
 * - Built-in LED: GPIO 8 (active LOW)
 * 
 * CAN-BUS:
 * - Baud Rate: 500 kbps
 * - Message ID: 0x101 (SHT31 temperature and humidity)
 * - Sends both temperature and humidity data with checksum
 * 
 * USAGE:
 * Upload to ESP32-C3 with SHT31 sensor and MCP2515 CAN controller.
 * Uses the sht31_sensor.h library for reliable sensor communication
 * and sends data via CAN-Bus to the main display unit.
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MCP2515.h>
#include "can_messages.h"
#include "sht31_sensor.h"

// CAN-Bus Configuration
Adafruit_MCP2515 can;

// LED Configuration
#define LED_PIN 8  // 8 on the C3, 2 on the ESP32

// Timing variables
static unsigned long last_send = 0;
static const unsigned long SEND_INTERVAL = 5000; // Send every 5 seconds
static unsigned long device_uptime = 0;
static uint16_t error_count = 0;

// Sensor configuration
static uint8_t sensor_id = 1;  // Unique identifier for this sensor
static uint8_t sensor_type = 1; // 1 = SHT31

// Function prototypes
void setupCAN();
void sendSensorData(float temperature, float humidity);
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
    Serial.println("🚌 Ready to send sensor data...");
}

void sendSensorData(float temperature, float humidity) {
    // Create SHT31 data message
    SHT31Data data;
    data.temperature_raw = temperatureToRaw(temperature);
    data.humidity_raw = humidityToRaw(humidity);
    data.sensor_id = sensor_id;
    data.status_flags = 0x03; // Both temperature and humidity valid
    
    // Calculate checksum
    data.checksum = calculateChecksum((uint8_t*)&data, sizeof(SHT31Data) - 2);
    
    // Send CAN message
    can_frame frame;
    frame.can_id = CAN_MSG_SHT31_TEMP_HUMIDITY;
    frame.can_dlc = sizeof(SHT31Data);
    memcpy(frame.data, &data, sizeof(SHT31Data));
    
    if (can.sendMessage(&frame) == MCP2515::ERROR_OK) {
        Serial.printf("📡 CAN SHT31 data sent - Temp: %.2f°C, Humidity: %.1f%%, Sensor ID: %d\n",
                     temperature, humidity, sensor_id);
    } else {
        Serial.println("❌ Failed to send CAN SHT31 data!");
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
    hb.device_type = 2; // 2 = SHT31 sensor
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
    Serial.println("🌡️ SHT31 CAN Sensor Client");
    Serial.println("============================");
    
    // Initialize LED pin and turn it ON
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // Turn LED ON for ESP32-C3
    Serial.println("💡 LED should be ON now");
    
    // Initialize the SHT31 sensor
    initSHT31();
    
    // Print sensor information
    printSensorInfo();
    
    // Setup CAN-Bus connection
    setupCAN();
    
    Serial.println("🚀 Starting temperature and humidity monitoring via CAN-Bus...");
    Serial.println();
}

void loop() {
    // Update SHT31 readings (handles timing internally)
    updateSHT31();
    
    // Handle LED status based on CAN communication
    static unsigned long last_led_toggle = 0;
    static bool led_state = false;
    
    // Slow flash (2 seconds) when running normally
    if (millis() - last_led_toggle >= 2000) {
        led_state = !led_state;
        digitalWrite(LED_PIN, led_state ? HIGH : LOW);
        last_led_toggle = millis();
    }
    
    // Send sensor data every 5 seconds
    if (millis() - last_send >= SEND_INTERVAL) {
        float current_temp = getCurrentTemperature();
        float current_humidity = getCurrentHumidity();
        
        // Validate sensor readings
        if (isTemperatureSafe() && isHumiditySafe()) {
            Serial.printf("📤 Sending CAN data - Temp: %.2f°C, Humidity: %.1f%%\n", 
                         current_temp, current_humidity);
            sendSensorData(current_temp, current_humidity);
            
            // Send sensor status every 5th message
            if ((millis() / SEND_INTERVAL) % 5 == 0) {
                sendSensorStatus();
            }
        } else {
            Serial.println("⚠️  Invalid sensor readings, skipping CAN transmission");
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
