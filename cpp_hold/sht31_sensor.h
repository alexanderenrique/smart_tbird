#ifndef SHT31_SENSOR_H
#define SHT31_SENSOR_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>

// Function declarations
void initSHT31();
void updateSHT31();
float getCurrentTemperature();
float getCurrentHumidity();
bool isTemperatureSafe();
bool isHumiditySafe();
const char* getTemperatureStatus();
const char* getHumidityStatus();
void printSensorInfo();

#endif 