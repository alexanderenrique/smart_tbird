/*
 * CAN Manager Implementation
 * =========================
 */

#include "can_manager.h"

// Global CAN manager instance
CANManager* g_can_manager = nullptr;

CANManager::CANManager(uint8_t id, uint8_t type) 
    : node_id(id), node_type(type), initialized(false), 
      last_heartbeat(0), last_error_report(0), message_count(0), error_count(0),
      queue_head(0), queue_tail(0), queue_count(0) {
}

bool CANManager::begin(uint8_t cs_pin, uint8_t int_pin) {
    Serial.println("Initializing CAN Manager...");
    
    // Initialize SPI
    SPI.begin();
    
    // Initialize MCP2515
    if (!mcp.begin_SPI(cs_pin, &SPI)) {
        Serial.println("ERROR: Failed to initialize MCP2515");
        return false;
    }
    
    // Set CAN speed to 500 kbps
    if (!mcp.setBitrate(CAN_500KBPS_TIMING)) {
        Serial.println("ERROR: Failed to set CAN bitrate");
        return false;
    }
    
    // Set to normal mode
    if (!mcp.setNormalMode()) {
        Serial.println("ERROR: Failed to set normal mode");
        return false;
    }
    
    initialized = true;
    Serial.printf("CAN Manager initialized successfully (Node ID: %d, Type: %d)\n", node_id, node_type);
    
    // Send initial heartbeat
    sendHeartbeat();
    
    return true;
}

bool CANManager::sendMessage(uint32_t id, const uint8_t* data, uint8_t length) {
    if (!initialized) {
        Serial.println("ERROR: CAN Manager not initialized");
        return false;
    }
    
    if (length > 8) {
        Serial.println("ERROR: Message too long (max 8 bytes)");
        return false;
    }
    
    // Try to send immediately
    if (mcp.sendMessage(id, data, length)) {
        message_count++;
        return true;
    }
    
    // If immediate send failed, queue the message
    if (queue_count < MAX_QUEUE_SIZE) {
        MessageQueue& msg = message_queue[queue_tail];
        msg.id = id;
        msg.length = length;
        msg.timestamp = millis();
        msg.retry_count = 0;
        memcpy(msg.data, data, length);
        
        queue_tail = (queue_tail + 1) % MAX_QUEUE_SIZE;
        queue_count++;
        
        Serial.printf("Message queued (ID: 0x%03X, Queue: %d/%d)\n", id, queue_count, MAX_QUEUE_SIZE);
        return true;
    }
    
    Serial.println("ERROR: Message queue full");
    error_count++;
    return false;
}

bool CANManager::sendSensorData(const SHT31Data& data) {
    return sendMessage(CAN_MSG_SHT31_TEMP_HUMIDITY, (const uint8_t*)&data, sizeof(data));
}

bool CANManager::sendSensorData(const TMP36Data& data) {
    return sendMessage(CAN_MSG_TMP36_TEMPERATURE, (const uint8_t*)&data, sizeof(data));
}

bool CANManager::sendStatus(const SensorStatus& status) {
    return sendMessage(CAN_MSG_SENSOR_STATUS, (const uint8_t*)&status, sizeof(status));
}

bool CANManager::sendHeartbeat() {
    Heartbeat hb;
    hb.device_type = node_type;
    hb.device_id = node_id;
    hb.uptime_seconds = millis() / 1000;
    hb.status_flags = initialized ? 0x01 : 0x00;
    hb.free_memory = ESP.getFreeHeap();
    hb.temperature = 25; // Placeholder - could read from internal sensor
    
    last_heartbeat = millis();
    return sendMessage(CAN_MSG_HEARTBEAT, (const uint8_t*)&hb, sizeof(hb));
}

bool CANManager::sendError(uint8_t error_code, const char* description) {
    // Create error report message
    uint8_t error_data[8];
    error_data[0] = node_type;
    error_data[1] = node_id;
    error_data[2] = error_code;
    error_data[3] = 0; // Reserved
    
    // Include description if provided (truncated to fit)
    if (description) {
        strncpy((char*)&error_data[4], description, 4);
    }
    
    last_error_report = millis();
    return sendMessage(CAN_MSG_ERROR_REPORT, error_data, 8);
}

bool CANManager::receiveMessage(uint32_t& id, uint8_t* data, uint8_t& length) {
    if (!initialized) return false;
    
    if (mcp.readMessage(&id, data, &length)) {
        return true;
    }
    
    return false;
}

bool CANManager::hasMessage() {
    if (!initialized) return false;
    return mcp.checkMessage();
}

void CANManager::processQueue() {
    if (queue_count == 0) return;
    
    MessageQueue& msg = message_queue[queue_head];
    
    // Try to send the message
    if (mcp.sendMessage(msg.id, msg.data, msg.length)) {
        // Success - remove from queue
        queue_head = (queue_head + 1) % MAX_QUEUE_SIZE;
        queue_count--;
        message_count++;
        
        Serial.printf("Queued message sent (ID: 0x%03X, Queue: %d/%d)\n", msg.id, queue_count, MAX_QUEUE_SIZE);
    } else {
        // Failed - increment retry count
        msg.retry_count++;
        
        // If too many retries, drop the message
        if (msg.retry_count >= 3) {
            Serial.printf("Dropping message after %d retries (ID: 0x%03X)\n", msg.retry_count, msg.id);
            queue_head = (queue_head + 1) % MAX_QUEUE_SIZE;
            queue_count--;
            error_count++;
        }
    }
}

void CANManager::clearQueue() {
    queue_head = 0;
    queue_tail = 0;
    queue_count = 0;
    Serial.println("Message queue cleared");
}

void CANManager::updateHeartbeat() {
    if (isHeartbeatDue()) {
        sendHeartbeat();
    }
}

bool CANManager::isHeartbeatDue() const {
    return (millis() - last_heartbeat) >= 30000; // 30 seconds
}

void CANManager::handleError(uint8_t error_code) {
    error_count++;
    reportError(error_code);
}

void CANManager::reportError(uint8_t error_code, const char* description) {
    sendError(error_code, description);
}

void CANManager::resetCounters() {
    message_count = 0;
    error_count = 0;
    Serial.println("Counters reset");
}

void CANManager::printStatus() {
    Serial.println("=== CAN Manager Status ===");
    Serial.printf("Node ID: %d, Type: %d\n", node_id, node_type);
    Serial.printf("Initialized: %s\n", initialized ? "Yes" : "No");
    Serial.printf("Messages Sent: %d\n", message_count);
    Serial.printf("Errors: %d\n", error_count);
    Serial.printf("Queue: %d/%d\n", queue_count, MAX_QUEUE_SIZE);
    Serial.printf("Free Memory: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Uptime: %lu seconds\n", millis() / 1000);
    Serial.println("==========================");
}

void CANManager::printQueueStatus() {
    if (queue_count == 0) {
        Serial.println("Message queue is empty");
        return;
    }
    
    Serial.printf("Message Queue (%d/%d):\n", queue_count, MAX_QUEUE_SIZE);
    for (uint8_t i = 0; i < queue_count; i++) {
        uint8_t idx = (queue_head + i) % MAX_QUEUE_SIZE;
        const MessageQueue& msg = message_queue[idx];
        Serial.printf("  [%d] ID: 0x%03X, Len: %d, Retries: %d, Age: %lu ms\n", 
                     i, msg.id, msg.length, msg.retry_count, millis() - msg.timestamp);
    }
}
