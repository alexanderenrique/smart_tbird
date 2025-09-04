/*
 * CAN Manager - Shared CAN Bus Communication Library
 * =================================================
 * 
 * This library provides a unified interface for CAN communication
 * across all nodes in the Smart Thunderbird network.
 * 
 * Features:
 * - Automatic CAN controller initialization
 * - Message queuing and transmission
 * - Error handling and recovery
 * - Heartbeat management
 * - Network diagnostics
 */

#ifndef CAN_MANAGER_H
#define CAN_MANAGER_H

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MCP2515.h>
#include "can_messages.h"

class CANManager {
private:
    Adafruit_MCP2515 mcp;
    uint8_t node_id;
    uint8_t node_type;
    bool initialized;
    unsigned long last_heartbeat;
    unsigned long last_error_report;
    uint16_t message_count;
    uint16_t error_count;
    
    // Message queue for reliable transmission
    struct MessageQueue {
        uint32_t id;
        uint8_t data[8];
        uint8_t length;
        unsigned long timestamp;
        uint8_t retry_count;
    };
    
    static const uint8_t MAX_QUEUE_SIZE = 10;
    MessageQueue message_queue[MAX_QUEUE_SIZE];
    uint8_t queue_head;
    uint8_t queue_tail;
    uint8_t queue_count;

public:
    CANManager(uint8_t id, uint8_t type);
    
    // Initialization
    bool begin(uint8_t cs_pin = CAN_CS_PIN, uint8_t int_pin = CAN_INT_PIN);
    bool isInitialized() const { return initialized; }
    
    // Message transmission
    bool sendMessage(uint32_t id, const uint8_t* data, uint8_t length);
    bool sendSensorData(const SHT31Data& data);
    bool sendSensorData(const TMP36Data& data);
    bool sendStatus(const SensorStatus& status);
    bool sendHeartbeat();
    bool sendError(uint8_t error_code, const char* description = nullptr);
    
    // Message reception
    bool receiveMessage(uint32_t& id, uint8_t* data, uint8_t& length);
    bool hasMessage();
    
    // Queue management
    void processQueue();
    void clearQueue();
    uint8_t getQueueCount() const { return queue_count; }
    
    // Network diagnostics
    uint16_t getMessageCount() const { return message_count; }
    uint16_t getErrorCount() const { return error_count; }
    void resetCounters();
    
    // Heartbeat management
    void updateHeartbeat();
    bool isHeartbeatDue() const;
    
    // Error handling
    void handleError(uint8_t error_code);
    void reportError(uint8_t error_code, const char* description = nullptr);
    
    // Utility functions
    void printStatus();
    void printQueueStatus();
};

// Global CAN manager instance (defined in .cpp file)
extern CANManager* g_can_manager;

// Convenience macros for common operations
#define CAN_SEND_SENSOR_DATA(data) g_can_manager->sendSensorData(data)
#define CAN_SEND_STATUS(status) g_can_manager->sendStatus(status)
#define CAN_SEND_HEARTBEAT() g_can_manager->sendHeartbeat()
#define CAN_SEND_ERROR(code, desc) g_can_manager->sendError(code, desc)
#define CAN_RECEIVE_MESSAGE(id, data, len) g_can_manager->receiveMessage(id, data, len)
#define CAN_HAS_MESSAGE() g_can_manager->hasMessage()

#endif // CAN_MANAGER_H
