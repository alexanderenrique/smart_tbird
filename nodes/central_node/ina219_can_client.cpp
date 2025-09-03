/*
 * INA219 CAN Client Implementation
 * ===============================
 * 
 * Implementation of CAN communication functions for INA219 battery monitoring.
 */

#include "ina219_can_client.h"
#include "../shared/can_lib/can_manager.h"

// External CAN manager reference
extern CANManager* g_can_manager;

// Send INA219 battery data via CAN
bool sendINA219BatteryData(const INA219Data& battery_data) {
    if (!g_can_manager) {
        Serial.println("ERROR: CAN manager not initialized");
        return false;
    }
    
    // Create CAN message
    uint32_t can_id = CAN_MSG_INA219_BATTERY;
    uint8_t can_data[8];
    
    // Pack data into CAN message
    can_data[0] = (battery_data.voltage_raw >> 8) & 0xFF;
    can_data[1] = battery_data.voltage_raw & 0xFF;
    can_data[2] = (battery_data.current_raw >> 8) & 0xFF;
    can_data[3] = battery_data.current_raw & 0xFF;
    can_data[4] = (battery_data.power_raw >> 8) & 0xFF;
    can_data[5] = battery_data.power_raw & 0xFF;
    can_data[6] = battery_data.sensor_id;
    can_data[7] = battery_data.status_flags;
    
    // Send message
    bool success = g_can_manager->sendMessage(can_id, can_data, 8);
    
    if (success) {
        Serial.printf("Sent INA219 data: %.3fV, %.3fA, %.3fW\n", 
                     rawToVoltage(battery_data.voltage_raw),
                     rawToCurrent(battery_data.current_raw),
                     rawToPower(battery_data.power_raw));
    } else {
        Serial.println("ERROR: Failed to send INA219 data via CAN");
    }
    
    return success;
}

// Handle INA219-specific CAN messages
void handleINA219CANMessages() {
    // Currently no specific INA219 CAN message handling needed
    // Battery data is sent periodically, not on request
}

// Print INA219 CAN status
void printINA219CANStatus() {
    Serial.println("=== INA219 CAN Status ===");
    Serial.printf("Message ID: 0x%03X\n", CAN_MSG_INA219_BATTERY);
    Serial.printf("Data Size: %d bytes\n", sizeof(INA219Data));
    Serial.printf("Transmission Rate: 0.2Hz (every 5 seconds)\n");
    Serial.println("=========================");
}
