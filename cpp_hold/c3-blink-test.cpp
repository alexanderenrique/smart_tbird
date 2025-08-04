/*
 * ESP32-C3 WiFi LED Blink Test
 * ============================
 * 
 * PURPOSE:
 * Simple test application for ESP32-C3 to verify WiFi connectivity and LED functionality.
 * Uses the built-in LED to indicate WiFi connection status with different blink patterns.
 * 
 * ENVIRONMENT:
 * - Hardware: ESP32-C3 development board
 * - Platform: PlatformIO with Arduino framework
 * - Libraries: WiFi
 * 
 * FUNCTIONALITY:
 * 1. Connects to WiFi network "SmartThunderbird" (password: 12345678)
 * 2. Uses built-in LED (GPIO 8) to show connection status:
 *    - Fast flash (100ms) while connecting
 *    - Very fast flash (50ms) when not connected
 *    - Very slow flash (2 seconds) when connected
 * 3. Continuously monitors WiFi status and updates LED pattern
 * 
 * CONNECTIONS:
 * - Built-in LED: GPIO 8 (active LOW - HIGH = off, LOW = on)
 * - WiFi: Built-in ESP32-C3 WiFi module
 * 
 * USAGE:
 * Use this to test WiFi connectivity and LED functionality on ESP32-C3.
 * LED blink pattern indicates WiFi connection status.
 */

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