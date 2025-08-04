/*
 * TMP36 Temperature Sensor Test
 * =============================
 * 
 * PURPOSE:
 * Simple test application for TMP36 temperature sensor to verify sensor functionality
 * and calibration. Reads and displays temperature data via serial output.
 * 
 * ENVIRONMENT:
 * - Hardware: ESP32 with TMP36 sensor
 * - Platform: PlatformIO with Arduino framework
 * - Libraries: tmp36_sensor.h
 * 
 * FUNCTIONALITY:
 * 1. Initializes TMP36 sensor using the sensor library
 * 2. Reads temperature data every 5 seconds
 * 3. Displays temperature readings via serial monitor
 * 4. Shows sensor information and calibration details
 * 5. Provides continuous temperature monitoring
 * 
 * CONNECTIONS:
 * - TMP36 VCC → ESP32 3.3V
 * - TMP36 GND → ESP32 GND
 * - TMP36 VOUT → ESP32 GPIO 32 (ADC1_CH0)
 * 
 * USAGE:
 * Use this to test TMP36 sensor functionality and verify temperature readings.
 * Monitor serial output to see temperature data and sensor status.
 */

#include <Arduino.h>
#include "tmp36_sensor.h"

void setup() {
  Serial.begin(9600);
  Serial.println("🌡️ TMP36 Temperature Sensor Test");
  Serial.println("==================================");
  
  // Initialize the TMP36 sensor
  initTMP36();
  
  // Print sensor information
  printSensorInfo();
  
  Serial.println("🚀 Starting temperature readings...");
  Serial.println();
}

void loop() {
  // Update temperature reading (handles timing internally)
  updateTemperature();
  
  // Small delay to prevent overwhelming the serial output
  delay(100);
} 