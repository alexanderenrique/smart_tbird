/*
 * SHT31 WiFi Sensor Client (Library Version)
 * ==========================================
 * 
 * PURPOSE:
 * Sensor client that reads SHT31 temperature and humidity data using the sht31_sensor.h
 * library and sends it via WiFi to the main display unit. More robust than simple version.
 * 
 * ENVIRONMENT:
 * - Hardware: ESP32-C3 with SHT31 sensor
 * - Platform: PlatformIO with Arduino framework
 * - Libraries: WiFi, HTTPClient, sht31_sensor.h
 * 
 * FUNCTIONALITY:
 * 1. Connects to WiFi network "SmartThunderbird" (password: 12345678)
 * 2. Initializes SHT31 sensor using the sensor library
 * 3. Reads temperature and humidity data with error handling
 * 4. Sends data via HTTP POST to main display unit every 5 seconds
 * 5. Uses built-in LED (GPIO 8) to indicate WiFi connection status
 * 6. Provides comprehensive serial debug output
 * 7. Includes automatic WiFi reconnection
 * 
 * CONNECTIONS:
 * - SHT31 VIN → ESP32 3.3V
 * - SHT31 GND → ESP32 GND
 * - SHT31 SDA → ESP32 GPIO 8
 * - SHT31 SCL → ESP32 GPIO 9
 * - Built-in LED: GPIO 8 (active LOW)
 * 
 * NETWORK:
 * - Server URL: http://192.168.4.1:8080/sensor
 * - Sends both temperature and humidity data
 * 
 * USAGE:
 * Upload to ESP32-C3 with SHT31 sensor. Uses the sht31_sensor.h library for
 * reliable sensor communication and sends data to the main display unit.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "sht31_sensor.h"

// WiFi Configuration - Connect to the AP
const char* ssid = "SmartThunderbird";
const char* password = "12345678";
const char* server_url = "http://192.168.4.1:8080/sensor";

// LED Configuration
#define LED_PIN 8  // 8 on the C3, 2 on the ESP32

// WiFi client
WiFiClient client;
HTTPClient http;

// Timing variables
static unsigned long last_send = 0;
static const unsigned long SEND_INTERVAL = 5000; // Send every 5 seconds

// Connection monitoring variables
static unsigned long last_connection_check = 0;
static const unsigned long CONNECTION_CHECK_INTERVAL = 10000; // Check every 10 seconds
static int connection_failures = 0;
static const int MAX_FAILURES = 5; // Reset after 5 failures

// Function prototypes
void setupWiFi();
void sendSensorData(float temperature, float humidity);

void setupWiFi() {
    Serial.println("📡 Connecting to WiFi AP...");
    Serial.printf("SSID: %s\n", ssid);
    Serial.println("📡 Connecting to Channel 6 (2.437 GHz)");
    
    // Start with LED off
    digitalWrite(LED_PIN, LOW);
    
    // Clear any old WiFi configurations
    WiFi.disconnect(true);  // Clear old config
    delay(1000);  // Give time for disconnect to complete
    
    // Configure WiFi power management for better stability
    WiFi.setSleep(false);  // Disable WiFi sleep for better connection stability
    WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Set maximum transmit power
    
    // Configure static IP - no DHCP
    IPAddress staticIP(192, 168, 4, 100);    // Static IP for this device
    IPAddress gateway(192, 168, 4, 1);       // Gateway (AP IP)
    IPAddress subnet(255, 255, 255, 0);      // Subnet mask
    IPAddress dns(192, 168, 4, 1);           // DNS server (same as gateway)
    
    // Configure WiFi with static IP
    WiFi.config(staticIP, gateway, subnet, dns);
    
    // Set WiFi mode and begin connection
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password, 6);  // Explicitly connect to channel 6
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        // Flash LED very fast while connecting
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(50);  // Flash every 50ms (very fast)
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("✅ WiFi connected!");
        Serial.printf("Static IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("Connected to Channel: %d\n", WiFi.channel());
        Serial.printf("Signal Strength (RSSI): %d dBm\n", WiFi.RSSI());
        Serial.printf("WiFi Power: %d dBm\n", WiFi.getTxPower());
        Serial.println("⚠️  Using static IP - DHCP disabled");
        Serial.println("💡 LED will now flash slowly (2 seconds) when connected");
        
        // Reset failure counter on successful connection
        connection_failures = 0;
    } else {
        Serial.println();
        Serial.println("❌ WiFi connection failed!");
        Serial.println("   Check if AP is running on Channel 6");
        Serial.println("   Verify SSID: " + String(ssid));
        connection_failures++;
    }
}

// Function to monitor WiFi connection health
void checkWiFiHealth() {
    if (millis() - last_connection_check >= CONNECTION_CHECK_INTERVAL) {
        last_connection_check = millis();
        
        if (WiFi.status() == WL_CONNECTED) {
            int rssi = WiFi.RSSI();
            Serial.printf("📊 WiFi Health Check - RSSI: %d dBm, Channel: %d\n", rssi, WiFi.channel());
            
            // Print detailed WiFi strength information
            Serial.printf("📡 WiFi Details - SSID: %s, IP: %s, Gateway: %s\n", 
                         WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str());
            
            // Signal strength interpretation
            if (rssi >= -50) {
                Serial.println("📶 Signal Strength: Excellent");
            } else if (rssi >= -60) {
                Serial.println("📶 Signal Strength: Good");
            } else if (rssi >= -70) {
                Serial.println("📶 Signal Strength: Fair");
            } else if (rssi >= -80) {
                Serial.println("📶 Signal Strength: Poor");
            } else {
                Serial.println("📶 Signal Strength: Very Poor");
            }
            
            // Warn if signal is getting weak
            if (rssi < -70) {
                Serial.println("⚠️  Warning: Weak WiFi signal detected!");
            }
        } else {
            Serial.println("❌ WiFi connection lost during health check!");
            connection_failures++;
            
            // If too many failures, try a complete reset
            if (connection_failures >= MAX_FAILURES) {
                Serial.println("🔄 Too many failures, performing complete WiFi reset...");
                WiFi.disconnect(true);
                delay(2000);
                connection_failures = 0;
                setupWiFi();
            }
        }
    }
}

void sendSensorData(float temperature, float humidity) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi not connected, attempting to reconnect...");
        Serial.println("💡 LED will flash fast while reconnecting...");
        
        // Clear old configurations before reconnecting
        WiFi.disconnect(true);
        delay(500);
        
        setupWiFi();
        return;
    }
    
    http.begin(client, server_url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.setTimeout(5000);  // 5 second timeout
    
    // Send both temperature and humidity data
    String post_data = "temp=" + String(temperature, 2) + "&humidity=" + String(humidity, 1);
    
    int httpResponseCode = http.POST(post_data);
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("📡 HTTP Response code: %d\n", httpResponseCode);
        Serial.printf("📡 Response: %s\n", response.c_str());
        
        // Check for successful response
        if (httpResponseCode == 200) {
            Serial.println("✅ Data sent successfully!");
        } else {
            Serial.printf("⚠️  Unexpected response code: %d\n", httpResponseCode);
        }
    } else {
        Serial.printf("❌ HTTP Error code: %d\n", httpResponseCode);
        Serial.printf("❌ Error: %s\n", http.errorToString(httpResponseCode).c_str());
        
        // Increment failure counter for HTTP errors
        connection_failures++;
    }
    
    http.end();
}

void setup() {
    Serial.begin(9600);
    Serial.println("🌡️ SHT31 WiFi Sensor Client");
    Serial.println("============================");
    
    // Initialize LED pin and turn it ON
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // Turn LED ON for ESP32-C3
    Serial.println("💡 LED should be ON now");
    
    // Initialize the SHT31 sensor
    initSHT31();
    
    // Print sensor information
    printSensorInfo();
    
    // Setup WiFi connection
    setupWiFi();
    
    Serial.println("🚀 Starting temperature and humidity monitoring...");
    Serial.println();
}

void loop() {
    // Update SHT31 readings (handles timing internally)
    updateSHT31();
    
    // Check WiFi health periodically
    checkWiFiHealth();
    
    // Handle LED status based on WiFi connection
    static unsigned long last_led_toggle = 0;
    static bool led_state = false;
    
    if (WiFi.status() == WL_CONNECTED) {
        // Slow flash (2 seconds) when connected
        if (millis() - last_led_toggle >= 2000) {
            led_state = !led_state;
            digitalWrite(LED_PIN, led_state ? HIGH : LOW);
            last_led_toggle = millis();
        }
    } else {
        // Fast flash when disconnected
        if (millis() - last_led_toggle >= 50) {
            led_state = !led_state;
            digitalWrite(LED_PIN, led_state ? HIGH : LOW);
            last_led_toggle = millis();
        }
    }
    
    // Send sensor data every 5 seconds
    if (millis() - last_send >= SEND_INTERVAL) {
        float current_temp = getCurrentTemperature();
        float current_humidity = getCurrentHumidity();
        
        // Print WiFi strength before sending data
        if (WiFi.status() == WL_CONNECTED) {
            int rssi = WiFi.RSSI();
            Serial.printf("📡 WiFi Strength: %d dBm | ", rssi);
        }
        
        Serial.printf("📤 Sending data - Temp: %.2f°C, Humidity: %.1f%%\n", 
                     current_temp, current_humidity);
        sendSensorData(current_temp, current_humidity);
        last_send = millis();
    }
    
    // Small delay to prevent overwhelming the system
    delay(100);
} 