#include <Arduino.h>

// TMP36 sensor pin configuration
#define TMP36_PIN 32  // ADC1_CH0 on ESP32
#define VREF 3.3      // ESP32 reference voltage
#define ADC_RESOLUTION 4095.0  // 12-bit ADC

// Temperature sensor variables
static float current_temp = 0.0;
static unsigned long last_reading = 0;
static const unsigned long READING_INTERVAL = 1000; // Read every 1 second

// Function to read temperature from TMP36
float readTemperature() {
    // Read analog value
    int adc_value = analogRead(TMP36_PIN);
    
    // Convert to voltage
    float voltage = (adc_value / ADC_RESOLUTION) * VREF;
    
    // Convert voltage to temperature (TMP36: 10mV/°C, 0.5V at 0°C)
    float temperature = (voltage - 0.5) * 100.0;
    
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
        current_temp = readTemperature();
        last_reading = millis();
        
        // Print temperature reading
        Serial.printf("🌡️ Temperature: %.2f°C (%.2f°F)\n", 
                     current_temp, (current_temp * 9.0/5.0) + 32.0);
        
        // Print status
        if (current_temp < 0) {
            Serial.println("❄️ Status: Cold");
        } else if (current_temp < 20) {
            Serial.println("🌤️ Status: Cool");
        } else if (current_temp < 30) {
            Serial.println("☀️ Status: Normal");
        } else if (current_temp < 40) {
            Serial.println("🔥 Status: Warm");
        } else {
            Serial.println("🌋 Status: Hot");
        }
        Serial.println("----------------------------------------");
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