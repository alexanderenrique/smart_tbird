/*
 * TMP36 Node - Temperature Sensor
 * ==============================
 * 
 * This node reads temperature from a TMP36 analog sensor
 * and transmits the data via CAN bus to the display node.
 * 
 * Features:
 * - TMP36 analog temperature sensor
 * - CAN bus communication
 * - Status LED indication
 * - Error handling and recovery
 */

#include <Arduino.h>

// Include shared libraries
#include "../shared/can_lib/can_messages.h"
#include "../shared/can_lib/can_manager.h"
#include "../shared/utils/node_config.h"

// CAN Manager
CANManager* can_manager = nullptr;

// Timing variables
unsigned long last_sensor_read = 0;
unsigned long last_heartbeat = 0;
unsigned long last_status_send = 0;

// Status variables
uint16_t error_count = 0;
uint16_t successful_reads = 0;

// ============================================================================
// SENSOR FUNCTIONS
// ============================================================================

float readTMP36Temperature() {
    // Read analog value
    int analog_value = analogRead(TMP36_ANALOG_PIN);
    
    // Convert to voltage
    float voltage = (analog_value / 4095.0) * TMP36_VOLTAGE_REF;
    
    // Convert to temperature (TMP36: 10mV/°C, 0.5V offset at 0°C)
    float temperature = (voltage - TMP36_OFFSET) * 100.0;
    
    return temperature;
}

bool readSensorData(TMP36Data& data) {
    float temperature = readTMP36Temperature();
    
    // Check for reasonable temperature range (-40°C to +125°C)
    if (temperature < -40.0 || temperature > 125.0) {
        Serial.printf("ERROR: Temperature out of range: %.1f°C\n", temperature);
        error_count++;
        return false;
    }
    
    // Convert to raw format
    data.temperature_raw = temperatureToRaw(temperature);
    data.sensor_id = TMP36_SENSOR_ID;
    data.status_flags = 0x01; // Temperature valid
    data.reserved = 0; // Reserved field
    
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
                    TMP36Data sensor_data;
                    if (readSensorData(sensor_data)) {
                        can_manager->sendSensorData(sensor_data);
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
    SensorStatus status;
    status.sensor_type = 2; // TMP36
    status.sensor_id = TMP36_SENSOR_ID;
    status.health_status = 0; // OK
    status.battery_level = 100; // Placeholder - could read from ADC
    status.uptime_seconds = millis() / 1000;
    status.error_count = error_count;
    
    can_manager->sendStatus(status);
}

void updateStatusLED() {
    static unsigned long last_led_update = 0;
    static bool led_state = false;
    
    if (millis() - last_led_update >= 2000) { // Toggle every 2 seconds
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
    
    Serial.println("=== Smart Thunderbird TMP36 Node ===");
    printNodeConfig();
    
    // Initialize analog pin
    pinMode(TMP36_ANALOG_PIN, INPUT);
    
    // Initialize status LED
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);
    
    // Test sensor reading
    float test_temp = readTMP36Temperature();
    Serial.printf("Initial temperature reading: %.1f°C\n", test_temp);
    
    // Initialize CAN manager
    can_manager = new CANManager(NODE_ID, NODE_TYPE);
    if (!can_manager->begin()) {
        Serial.println("ERROR: Failed to initialize CAN manager");
        return;
    }
    
    g_can_manager = can_manager;
    
    // Send initial status
    sendStatusUpdate();
    
    Serial.println("TMP36 node initialized successfully");
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
        TMP36Data sensor_data;
        if (readSensorData(sensor_data)) {
            // Send sensor data via CAN
            if (can_manager->sendSensorData(sensor_data)) {
                Serial.printf("Sent temperature: %.1f°C\n", 
                             rawToTemperature(sensor_data.temperature_raw));
            } else {
                Serial.println("ERROR: Failed to send sensor data");
                error_count++;
            }
        }
        
        last_sensor_read = current_time;
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
    
    // Print status every 30 seconds
    static unsigned long last_status_print = 0;
    if (current_time - last_status_print >= 30000) {
        Serial.printf("Status: Reads=%d, Errors=%d, Queue=%d\n", 
                     successful_reads, error_count, can_manager->getQueueCount());
        last_status_print = current_time;
    }
    
    delay(100);
}
