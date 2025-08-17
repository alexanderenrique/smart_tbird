/*
 * CAN-Bus Test Program
 * ====================
 * 
 * PURPOSE:
 * Simple test program to verify CAN-Bus setup and communication.
 * Can be used to test MCP2515 connections and basic CAN functionality.
 * 
 * ENVIRONMENT:
 * - Hardware: ESP32 with MCP2515 CAN controller
 * - Platform: PlatformIO with Arduino framework
 * - Libraries: SPI, Adafruit_MCP2515
 * 
 * FUNCTIONALITY:
 * 1. Initializes MCP2515 CAN controller
 * 2. Sends test messages every 2 seconds
 * 3. Receives and displays any incoming messages
 * 4. Provides comprehensive debug output
 * 
 * CONNECTIONS:
 * - MCP2515 CS → ESP32 GPIO 5
 * - MCP2515 INT → ESP32 GPIO 2
 * - MCP2515 CLK → ESP32 GPIO 18
 * - MCP2515 MOSI → ESP32 GPIO 23
 * - MCP2515 MISO → ESP32 GPIO 19
 * 
 * USAGE:
 * Upload to ESP32 with MCP2515 to test basic CAN functionality.
 * Useful for debugging hardware connections and basic communication.
 */

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MCP2515.h>

// CAN-Bus Configuration
Adafruit_MCP2515 can;

// Test message counter
static uint32_t message_counter = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("🚌 CAN-Bus Test Program");
    Serial.println("========================");
    
    // Initialize SPI for MCP2515
    SPI.begin(18, 19, 23, 5); // CLK, MISO, MOSI, CS
    
    Serial.println("🚌 Initializing CAN controller...");
    
    // Initialize MCP2515 CAN controller
    if (!can.begin(500000)) { // 500 kbps
        Serial.println("❌ Failed to initialize CAN controller!");
        Serial.println("   Check MCP2515 connections:");
        Serial.println("   CS: GPIO 5");
        Serial.println("   INT: GPIO 2");
        Serial.println("   CLK: GPIO 18");
        Serial.println("   MOSI: GPIO 23");
        Serial.println("   MISO: GPIO 19");
        Serial.println("   Power: 3.3V and GND");
        return;
    }
    
    Serial.println("✅ CAN controller initialized successfully!");
    Serial.println("🚌 Speed: 500 kbps");
    Serial.println("🚌 Ready to send/receive messages...");
    Serial.println();
}

void loop() {
    // Send test message every 2 seconds
    static unsigned long last_send = 0;
    if (millis() - last_send >= 2000) {
        // Create test message
        can_frame frame;
        frame.can_id = 0x123; // Test message ID
        frame.can_dlc = 8;     // 8 bytes
        
        // Fill with test data
        frame.data[0] = (message_counter >> 24) & 0xFF;
        frame.data[1] = (message_counter >> 16) & 0xFF;
        frame.data[2] = (message_counter >> 8) & 0xFF;
        frame.data[3] = message_counter & 0xFF;
        frame.data[4] = 0xAA; // Test pattern
        frame.data[5] = 0x55; // Test pattern
        frame.data[6] = 0xAA; // Test pattern
        frame.data[7] = 0x55; // Test pattern
        
        // Send message
        if (can.sendMessage(&frame) == MCP2515::ERROR_OK) {
            Serial.printf("📤 Sent test message #%lu (ID: 0x%03X)\n", message_counter, frame.can_id);
            message_counter++;
        } else {
            Serial.println("❌ Failed to send test message!");
        }
        
        last_send = millis();
    }
    
    // Check for received messages
    can_frame received_frame;
    if (can.readMessage(&received_frame) == MCP2515::ERROR_OK) {
        Serial.printf("📥 Received message (ID: 0x%03X, Length: %d): ", 
                     received_frame.can_id, received_frame.can_dlc);
        
        // Print data bytes
        for (int i = 0; i < received_frame.can_dlc; i++) {
            Serial.printf("%02X ", received_frame.data[i]);
        }
        Serial.println();
    }
    
    delay(100);
}
