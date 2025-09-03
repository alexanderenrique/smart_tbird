/*
 * TMP36 Temperature Sensor Library
 * ================================
 * 
 * PURPOSE:
 * Library for reading temperature data from TMP36 analog temperature sensor.
 * Provides functions for sensor initialization, data reading, and status monitoring.
 * 
 * ENVIRONMENT:
 * - Hardware: ESP32 with TMP36 sensor
 * - Platform: PlatformIO with Arduino framework
 * - Libraries: Arduino (ADC functions)
 * 
 * FUNCTIONALITY:
 * 1. Initializes ADC for TMP36 sensor reading
 * 2. Reads temperature data every 5 seconds
 * 3. Converts analog voltage to temperature (10mV/°C, 0.5V at 0°C)
 * 4. Applies calibration offset for accuracy
 * 5. Provides status indicators for temperature ranges
 * 6. Includes safety checks and error handling
 * 7. Serial debug output with raw ADC and voltage values
 * 
 * CONNECTIONS:
 * - TMP36 VCC → ESP32 3.3V
 * - TMP36 GND → ESP32 GND
 * - TMP36 VOUT → ESP32 GPIO 32 (ADC1_CH0)
 * 
 * SENSOR SPECS:
 * - Temperature Range: -40°C to +125°C
 * - Output: 10mV/°C linear scale
 * - 0.5V output at 0°C
 * - Accuracy: ±2°C (typical)
 * - Supply Voltage: 2.7V to 5.5V
 * 
 * CALIBRATION:
 * - TEMP_OFFSET: 11.41°C (adjust based on your sensor)
 * - ADC Resolution: 12-bit (0-4095)
 * - Reference Voltage: 3.3V
 * 
 * USAGE:
 * Include this library in sensor projects that need temperature data.
 * Call initTMP36() in setup() and updateTemperature() in loop().
 */

#include <Arduino.h>

// TMP36 sensor pin configuration
#define TMP36_PIN 32  // ADC1_CH0 on ESP32
#define VREF 3.3      // ESP32 reference voltage
#define ADC_RESOLUTION 4095.0  // 12-bit ADC

// Temperature calibration offset (adjust this based on your sensor)
#define TEMP_OFFSET 11.41  // Offset in degrees Celsius

// Temperature sensor variables
static float current_temp = 0.0;
static unsigned long last_reading = 0;
static const unsigned long READING_INTERVAL = 5000; // Read every 5 seconds

// Function to read temperature from TMP36
float readTemperature() {
    // Read analog value
    int adc_value = analogRead(TMP36_PIN);
    
    // Convert to voltage
    float voltage = (adc_value / ADC_RESOLUTION) * VREF;
    
    // Convert voltage to temperature (TMP36: 10mV/°C, 0.5V at 0°C)
    float temperature = (voltage - 0.5) * 100.0;
    
    // Apply calibration offset
    temperature += TEMP_OFFSET;
    
    return temperature;
}

// Function to initialize TMP36 sensor
void initTMP36() {
    // Configure ADC
    analogReadResolution(12);  // Set ADC resolution to 12 bits
    analogSetAttenuation(ADC_11db);  // Set attenuation for 0-3.3V range
    
    Serial.println("🌡️ TMP36 temperature sensor initialized");
    Serial.println("📊 Reading temperature every second...");
    Serial.println("----------------------------------------");
}

// Function to update temperature reading
void updateTemperature() {
    if (millis() - last_reading >= READING_INTERVAL) {
        // Read analog value and calculate voltage
        int adc_value = analogRead(TMP36_PIN);
        float voltage = (adc_value / ADC_RESOLUTION) * VREF;
        
        current_temp = readTemperature();
        last_reading = millis();
        
        // Print raw values for debugging
        Serial.printf("📊 Raw ADC: %d | Voltage: %.3fV\n", adc_value, voltage);
        
        // Print temperature reading
        Serial.printf("🌡️ Temperature: %.2f°C (%.2f°F)\n", 
                     current_temp, (current_temp * 9.0/5.0) + 32.0);
        
    }
}

// Function to get current temperature (for external use)
float getCurrentTemperature() {
    return current_temp;
}

// Function to check if temperature is in safe range
bool isTemperatureSafe() {
    return (current_temp >= -40.0 && current_temp <= 125.0);
}

// Function to get temperature status string
const char* getTemperatureStatus() {
    if (current_temp < 0) return "Cold";
    else if (current_temp < 20) return "Cool";
    else if (current_temp < 30) return "Normal";
    else if (current_temp < 40) return "Warm";
    else return "Hot";
}

// Function to print sensor info
void printSensorInfo() {
    Serial.println("📋 TMP36 Sensor Information:");
    Serial.printf("   Pin: GPIO %d\n", TMP36_PIN);
    Serial.printf("   Reference Voltage: %.1fV\n", VREF);
    Serial.printf("   ADC Resolution: %.0f bits\n", log(ADC_RESOLUTION + 1) / log(2));
    Serial.printf("   Safe Range: -40°C to +125°C\n");
    Serial.println("----------------------------------------");
} 