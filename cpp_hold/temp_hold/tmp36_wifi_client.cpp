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
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
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