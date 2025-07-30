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