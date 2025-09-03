/*
 * MPU6050 CAN Client
 * =================
 * 
 * This module handles CAN bus communication for MPU6050 IMU data.
 * It provides functions to send sensor data and max values to the display node.
 */

#include <Arduino.h>
#include "../shared/can_lib/can_messages.h"
#include "../shared/can_lib/can_manager.h"
#include "mpu6050_sensor.h"

// External CAN manager instance
extern CANManager* g_can_manager;

// Send MPU6050 sensor data via CAN
bool sendMPU6050Data(const MPU6050Data& data) {
    if (!g_can_manager) {
        Serial.println("ERROR: CAN manager not initialized");
        return false;
    }
    
    // Create CAN message
    uint32_t can_id = CAN_MSG_MPU6050_IMU;
    uint8_t can_data[8];
    
    // Pack data into CAN message (8 bytes)
    can_data[0] = (data.accel_x >> 8) & 0xFF;  // High byte
    can_data[1] = data.accel_x & 0xFF;         // Low byte
    can_data[2] = (data.accel_y >> 8) & 0xFF;  // High byte
    can_data[3] = data.accel_y & 0xFF;         // Low byte
    can_data[4] = (data.accel_z >> 8) & 0xFF;  // High byte
    can_data[5] = data.accel_z & 0xFF;         // Low byte
    can_data[6] = data.sensor_id;
    can_data[7] = data.status_flags;
    
    // Send via CAN
    bool success = g_can_manager->sendMessage(can_id, can_data, 8);
    
    if (success) {
        Serial.printf("Sent MPU6050 data: X=%.2fg, Y=%.2fg, Z=%.2fg\n",
                     rawToAcceleration(data.accel_x),
                     rawToAcceleration(data.accel_y),
                     rawToAcceleration(data.accel_z));
    } else {
        Serial.println("ERROR: Failed to send MPU6050 data via CAN");
    }
    
    return success;
}

// Send MPU6050 max values via CAN
bool sendMPU6050MaxValues(const MPU6050MaxData& max_data) {
    if (!g_can_manager) {
        Serial.println("ERROR: CAN manager not initialized");
        return false;
    }
    
    // Create CAN message for max values
    uint32_t can_id = CAN_MSG_MPU6050_IMU + 1; // Use next ID for max values
    uint8_t can_data[8];
    
    // Pack max data into CAN message (8 bytes)
    can_data[0] = (max_data.max_accel_x >> 8) & 0xFF;  // High byte
    can_data[1] = max_data.max_accel_x & 0xFF;         // Low byte
    can_data[2] = (max_data.max_accel_y >> 8) & 0xFF;  // High byte
    can_data[3] = max_data.max_accel_y & 0xFF;         // Low byte
    can_data[4] = (max_data.max_accel_z >> 8) & 0xFF;  // High byte
    can_data[5] = max_data.max_accel_z & 0xFF;         // Low byte
    can_data[6] = max_data.sensor_id;
    can_data[7] = max_data.status_flags;
    
    // Send via CAN
    bool success = g_can_manager->sendMessage(can_id, can_data, 8);
    
    if (success) {
        Serial.printf("Sent MPU6050 absolute max values: X=%.2fg, Y=%.2fg, Z=%.2fg (Manual resets: %d)\n",
                     rawToAcceleration(max_data.max_accel_x),
                     rawToAcceleration(max_data.max_accel_y),
                     rawToAcceleration(max_data.max_accel_z),
                     max_data.reset_counter);
    } else {
        Serial.println("ERROR: Failed to send MPU6050 max values via CAN");
    }
    
    return success;
}

// Handle incoming CAN messages for MPU6050
void handleMPU6050CANMessages() {
    if (!g_can_manager) {
        return;
    }
    
    uint32_t id;
    uint8_t data[8];
    uint8_t length;
    
    while (g_can_manager->hasMessage()) {
        if (g_can_manager->receiveMessage(id, data, length)) {
            switch (id) {
                case CAN_MSG_DISPLAY_REQUEST:
                    // Display is requesting sensor data
                    Serial.println("Received data request from display for MPU6050");
                    // The main loop will handle sending current data
                    break;
                    
                case CAN_MSG_SYSTEM_RESET:
                    Serial.println("Received system reset command");
                    ESP.restart();
                    break;
                    
                case CAN_MSG_HEARTBEAT:
                    // Acknowledge heartbeat from display
                    break;
                    
                default:
                    // Unknown message ID
                    break;
            }
        }
    }
}

// Print MPU6050 CAN client status
void printMPU6050CANStatus() {
    if (g_can_manager) {
        Serial.printf("MPU6050 CAN Status: Queue=%d, Messages sent=%d\n",
                     g_can_manager->getQueueCount(),
                     g_can_manager->getMessagesSent());
    } else {
        Serial.println("MPU6050 CAN Status: Manager not initialized");
    }
}
