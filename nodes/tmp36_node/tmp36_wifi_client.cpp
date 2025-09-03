/*
 * TMP36 WiFi Sensor Client
 * ========================
 * 
 * PURPOSE:
 * Sensor client that reads TMP36 temperature data and sends it via WiFi to the main
 * display unit. Uses the tmp36_sensor.h library for reliable sensor communication.
 * 
 * ENVIRONMENT:
 * - Hardware: ESP32 with TMP36 sensor
 * - Platform: PlatformIO with Arduino framework
 * - Libraries: WiFi, HTTPClient, tmp36_sensor.h
 * 
 * FUNCTIONALITY:
 * 1. Connects to WiFi network "SmartThunderbird" (password: 12345678)
 * 2. Initializes TMP36 sensor using the sensor library
 * 3. Reads temperature data with ADC conversion and calibration
 * 4. Sends temperature data via HTTP POST to main display unit every 5 seconds
 * 5. Provides comprehensive serial debug output
 * 6. Includes automatic WiFi reconnection
 * 
 * CONNECTIONS:
 * - TMP36 VCC → ESP32 3.3V
 * - TMP36 GND → ESP32 GND
 * - TMP36 VOUT → ESP32 GPIO 32 (ADC1_CH0)
 * 
 * NETWORK:
 * - Server URL: http://192.168.4.1:8080/sensor
 * - Sends temperature data only (no humidity)
 * 
 * USAGE:
 * Upload to ESP32 with TMP36 sensor. Uses the tmp36_sensor.h library for
 * reliable sensor communication and sends temperature data to the main display unit.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "tmp36_sensor.h"

// WiFi Configuration - Connect to the AP
const char* ssid = "SmartThunderbird";
const char* password = "12345678";
const char* server_url = "http://192.168.4.1:8080/sensor";

// WiFi client
WiFiClient client;
HTTPClient http;

// Timing variables
static unsigned long last_send = 0;
static const unsigned long SEND_INTERVAL = 5000; // Send every 5 seconds

void setupWiFi() {
    Serial.println("📡 Connecting to WiFi AP...");
    Serial.printf("SSID: %s\n", ssid);
    
    // Configure static IP - no DHCP
    IPAddress staticIP(192, 168, 4, 102);    // Static IP for this device
    IPAddress gateway(192, 168, 4, 1);       // Gateway (AP IP)
    IPAddress subnet(255, 255, 255, 0);      // Subnet mask
    IPAddress dns(192, 168, 4, 1);           // DNS server (same as gateway)
    
    // Configure WiFi with static IP
    WiFi.config(staticIP, gateway, subnet, dns);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("✅ WiFi connected!");
        Serial.printf("Static IP Address: %s\n", WiFi.localIP().toString().c_str());
        Serial.println("⚠️  Using static IP - DHCP disabled");
    } else {
        Serial.println();
        Serial.println("❌ WiFi connection failed!");
    }
}

void sendTemperatureData(float temperature) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi not connected, attempting to reconnect...");
        setupWiFi();
        return;
    }
    
    http.begin(client, server_url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String post_data = "temp=" + String(temperature, 2);
    
    int httpResponseCode = http.POST(post_data);
    
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.printf("📡 HTTP Response code: %d\n", httpResponseCode);
        Serial.printf("📡 Response: %s\n", response.c_str());
    } else {
        Serial.printf("❌ HTTP Error code: %d\n", httpResponseCode);
    }
    
    http.end();
}

void setup() {
    Serial.begin(9600);
    Serial.println("🌡️ TMP36 WiFi Sensor Client");
    Serial.println("=============================");
    
    // Initialize the TMP36 sensor
    initTMP36();
    
    // Print sensor information
    printSensorInfo();
    
    // Setup WiFi connection
    setupWiFi();
    
    Serial.println("🚀 Starting temperature monitoring and sending...");
    Serial.println();
}

void loop() {
    // Update temperature reading (handles timing internally)
    updateTemperature();
    
    // Send temperature data every 5 seconds
    if (millis() - last_send >= SEND_INTERVAL) {
        float current_temp = getCurrentTemperature();
        Serial.printf("📤 Sending temperature: %.2f°C\n", current_temp);
        sendTemperatureData(current_temp);
        last_send = millis();
    }
    
    // Small delay to prevent overwhelming the system
    delay(100);
} 