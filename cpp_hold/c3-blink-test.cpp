#include <Arduino.h>
#include <WiFi.h>

// LED Configuration for ESP32-C3
#define LED_PIN 8  // Built-in LED on ESP32-C3

// WiFi Configuration
const char* ssid = "SmartThunderbird";
const char* password = "12345678";

void setupWiFi() {
    // Start with LED off (HIGH for active LOW LED)
    digitalWrite(LED_PIN, HIGH);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        // Flash LED while connecting (fast flash)
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(100);  // Flash every 100ms
        attempts++;
    }
}

void setup() {
    // Initialize LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // Start with LED off (active LOW)
    
    // Setup WiFi connection
    setupWiFi();
}

void loop() {
    // Check WiFi status and update LED accordingly
    if (WiFi.status() != WL_CONNECTED) {
        // Very fast flash when not connected
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(50);  // Flash every 50ms (very fast)
    } else {
        // Very slow flash when connected
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(2000);  // Flash every 2 seconds (very slow)
    }
}