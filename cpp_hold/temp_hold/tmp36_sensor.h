#ifndef TMP36_SENSOR_H
#define TMP36_SENSOR_H

#include <Arduino.h>

// Function declarations
float readTemperature();
void updateTemperature();
void initTMP36();
float getCurrentTemperature();
bool isTemperatureSafe();
const char* getTemperatureStatus();
void printSensorInfo();

#endif // TMP36_SENSOR_H 