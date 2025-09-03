/*
 * Central Node - SHT31 + MPU6050 Sensors
 * =====================================
 * 
 * This node reads temperature and humidity from an SHT31 sensor
 * and acceleration/gyroscope data from an MPU6050 6-axis IMU.
 * All data is transmitted via CAN bus to the display node.
 * 
 * Features:
 * - SHT31 temperature and humidity sensor
 * - MPU6050 6-axis IMU with absolute max G-force tracking (100Hz sampling)
 * - Digital Low-Pass Filtering (DLPF) for noise reduction
 * - CAN bus communication
 * - Status LED indication
 * - Error handling and recovery
 * - Automotive G-force monitoring (acceleration, braking, cornering)
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>

// Include shared libraries
#include "../shared/can_lib/can_messages.h"
#include "../shared/can_lib/can_manager.h"
#include "../shared/utils/node_config.h"

// Include sensor classes
#include "mpu6050_sensor.h"
#include "mpu6050_can_client.h"

// SHT31 sensor object
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// MPU6050 sensor object
MPU6050Sensor mpu6050(MPU6050_ADDRESS, MPU6050_SDA_PIN, MPU6050_SCL_PIN);

// CAN Manager
CANManager* can_manager = nullptr;

// Timing variables
unsigned long last_sensor_read = 0;
unsigned long last_heartbeat = 0;
unsigned long last_status_send = 0;
unsigned long last_mpu6050_max_send = 0;

// Status variables
bool sht31_initialized = false;
bool mpu6050_initialized = false;
uint16_t error_count = 0;
uint16_t successful_reads = 0;

// ============================================================================
// SENSOR FUNCTIONS
// ============================================================================

bool initializeSHT31() {
    Serial.println("Initializing SHT31 sensor...");
    
    if (!sht31.begin(SHT31_ADDRESS)) {
        Serial.println("ERROR: Could not find SHT31 sensor");
        return false;
    }
    
    // Test sensor communication
    float temp = sht31.readTemperature();
    float humidity = sht31.readHumidity();
    
    if (isnan(temp) || isnan(humidity)) {
        Serial.println("ERROR: SHT31 sensor communication failed");
        return false;
    }
    
    Serial.printf("SHT31 initialized successfully (Temp: %.1f°C, Humidity: %.1f%%)\n", temp, humidity);
    return true;
}

bool initializeMPU6050() {
    Serial.println("Initializing MPU6050 sensor...");
    
    if (!mpu6050.begin()) {
        Serial.println("ERROR: Could not initialize MPU6050 sensor");
        return false;
    }
    
    Serial.println("MPU6050 initialized successfully");
    return true;
}

bool readSensorData(SHT31Data& data) {
    float temperature = sht31.readTemperature();
    float humidity = sht31.readHumidity();
    
    // Check for valid readings
    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("ERROR: Invalid sensor readings");
        error_count++;
        return false;
    }
    
    // Convert to raw format
    data.temperature_raw = temperatureToRaw(temperature);
    data.humidity_raw = humidityToRaw(humidity);
    data.sensor_id = SHT31_SENSOR_ID;
    data.status_flags = 0x03; // Both temperature and humidity valid
    
    // Calculate checksum
    data.checksum = calculateChecksum((const uint8_t*)&data, sizeof(data));
    
    successful_reads++;
    return true;
}

// ============================================================================
// CAN MESSAGE HANDLING
// ============================================================================

void handleCANMessages() {
    uint32_t id;
    uint8_t data[8];
    uint8_t length;

    while (can_manager->hasMessage()) {
        if (can_manager->receiveMessage(id, data, length)) {
            switch (id) {
                case CAN_MSG_DISPLAY_REQUEST:
                    Serial.println("Received data request from display");
                    // Immediately send sensor data
                    if (sht31_initialized) {
                        SHT31Data sht31_data;
                        if (readSensorData(sht31_data)) {
                            can_manager->sendSensorData(sht31_data);
                        }
                    }
                    if (mpu6050_initialized) {
                        MPU6050Data mpu6050_data;
                        if (mpu6050.readSensorData(mpu6050_data)) {
                            sendMPU6050Data(mpu6050_data);
                        }
                    }
                    break;
                    
                case CAN_MSG_SYSTEM_RESET:
                    Serial.println("Received system reset command");
                    ESP.restart();
                    break;
                    
                case CAN_MSG_HEARTBEAT:
                    // Acknowledge heartbeat from display
                    break;
            }
        }
    }
}

// ============================================================================
// STATUS FUNCTIONS
// ============================================================================

void sendStatusUpdate() {
    // Send SHT31 status
    SensorStatus sht31_status;
    sht31_status.sensor_type = 1; // SHT31
    sht31_status.sensor_id = SHT31_SENSOR_ID;
    sht31_status.health_status = sht31_initialized ? 0 : 2; // 0=OK, 2=Error
    sht31_status.battery_level = 100; // Placeholder - could read from ADC
    sht31_status.uptime_seconds = millis() / 1000;
    sht31_status.error_count = error_count;
    
    can_manager->sendStatus(sht31_status);
    
    // Send MPU6050 status
    SensorStatus mpu6050_status;
    mpu6050_status.sensor_type = 3; // MPU6050
    mpu6050_status.sensor_id = MPU6050_SENSOR_ID;
    mpu6050_status.health_status = mpu6050_initialized ? 0 : 2; // 0=OK, 2=Error
    mpu6050_status.battery_level = 100; // Placeholder - could read from ADC
    mpu6050_status.uptime_seconds = millis() / 1000;
    mpu6050_status.error_count = mpu6050.getErrorCount();
    
    can_manager->sendStatus(mpu6050_status);
}

void updateStatusLED() {
    static unsigned long last_led_update = 0;
    static bool led_state = false;
    
    if (millis() - last_led_update >= 1000) { // Toggle every second
        led_state = !led_state;
        digitalWrite(STATUS_LED_PIN, led_state ? HIGH : LOW);
        last_led_update = millis();
    }
}

// ============================================================================
// MAIN SETUP
// ============================================================================

void setup() {
    Serial.begin(9600);
    delay(1000);
    
    Serial.println("=== Smart Thunderbird Central Node ===");
    printNodeConfig();
    
    // Initialize I2C
    Wire.begin(SHT31_SDA_PIN, SHT31_SCL_PIN);
    
    // Initialize status LED
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);
    
    // Initialize sensors
    sht31_initialized = initializeSHT31();
    mpu6050_initialized = initializeMPU6050();
    
    // Initialize CAN manager
    can_manager = new CANManager(NODE_ID, NODE_TYPE);
    if (!can_manager->begin()) {
        Serial.println("ERROR: Failed to initialize CAN manager");
        return;
    }
    
    g_can_manager = can_manager;
    
    // Send initial status
    sendStatusUpdate();
    
    Serial.println("Central node initialized successfully");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    unsigned long current_time = millis();
    
    // Handle CAN messages
    handleCANMessages();
    
    // Read sensor data
    if (current_time - last_sensor_read >= SENSOR_READ_INTERVAL) {
        // Read SHT31 data
        if (sht31_initialized) {
            SHT31Data sht31_data;
            if (readSensorData(sht31_data)) {
                // Send sensor data via CAN
                if (can_manager->sendSensorData(sht31_data)) {
                    Serial.printf("Sent SHT31 data: %.1f°C, %.1f%%\n", 
                                 rawToTemperature(sht31_data.temperature_raw),
                                 rawToHumidity(sht31_data.humidity_raw));
                } else {
                    Serial.println("ERROR: Failed to send SHT31 data");
                    error_count++;
                }
            } else {
                // Try to reinitialize sensor
                sht31_initialized = initializeSHT31();
            }
        } else {
            // Try to initialize sensor
            sht31_initialized = initializeSHT31();
        }
        
        // Read MPU6050 data
        if (mpu6050_initialized) {
            MPU6050Data mpu6050_data;
            if (mpu6050.readSensorData(mpu6050_data)) {
                // Send sensor data via CAN
                if (sendMPU6050Data(mpu6050_data)) {
                    // Data sent successfully
                } else {
                    Serial.println("ERROR: Failed to send MPU6050 data");
                    error_count++;
                }
            } else {
                // Try to reinitialize sensor
                mpu6050_initialized = initializeMPU6050();
            }
        } else {
            // Try to initialize sensor
            mpu6050_initialized = initializeMPU6050();
        }
        
        last_sensor_read = current_time;
    }
    
    // Send MPU6050 max values every 2 seconds (absolute max since power-on)
    if (current_time - last_mpu6050_max_send >= 2000) { // Every 2 seconds
        if (mpu6050_initialized) {
            MPU6050MaxData max_data;
            if (mpu6050.readMaxValues(max_data)) {
                sendMPU6050MaxValues(max_data);
            }
        }
        last_mpu6050_max_send = current_time;
    }
    
    // Send status update
    if (current_time - last_status_send >= 25000) { // Every 25 seconds
        sendStatusUpdate();
        last_status_send = current_time;
    }
    
    // Send heartbeat
    if (current_time - last_heartbeat >= HEARTBEAT_INTERVAL) {
        can_manager->sendHeartbeat();
        last_heartbeat = current_time;
    }
    
    // Update status LED
    updateStatusLED();
    
    // Process CAN message queue
    can_manager->processQueue();
    
    // Print status every 60 seconds (less frequent due to high sample rate)
    static unsigned long last_status_print = 0;
    if (current_time - last_status_print >= 60000) {
        Serial.printf("Status: Reads=%d, Errors=%d, Queue=%d\n", 
                     successful_reads, error_count, can_manager->getQueueCount());
        Serial.printf("SHT31: %s, MPU6050: %s\n", 
                     sht31_initialized ? "OK" : "ERROR",
                     mpu6050_initialized ? "OK" : "ERROR");
        if (mpu6050_initialized) {
            mpu6050.printStatus();
        }
        last_status_print = current_time;
    }
    
    delay(5); // Reduced delay for 100Hz operation
}
