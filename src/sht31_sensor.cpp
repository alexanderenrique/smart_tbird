#include "sht31_sensor.h"

// I2C pin configuration (ESP32 defaults)
#define SDA_PIN 21  // I2C Data pin
#define SCL_PIN 22  // I2C Clock pin

// SHT31 sensor instance
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Temperature and humidity variables
static float current_temp = 0.0;
static float current_humidity = 0.0;
static unsigned long last_reading = 0;
static const unsigned long READING_INTERVAL = 5000; // Read every 2 seconds

// Function to initialize SHT31 sensor
void initSHT31() {
    // Initialize I2C communication with specific pins
    Wire.begin(SDA_PIN, SCL_PIN);
    
    Serial.printf("🔌 I2C initialized - SDA: GPIO%d, SCL: GPIO%d\n", SDA_PIN, SCL_PIN);
    
    // Initialize SHT31 sensor
    if (!sht31.begin(0x44)) {  // Try default address 0x44
        Serial.println("❌ SHT31 sensor not found at address 0x44, trying 0x45...");
        if (!sht31.begin(0x45)) {  // Try alternate address 0x45
            Serial.println("❌ SHT31 sensor not found! Check wiring.");
            Serial.printf("   Expected connections:\n");
            Serial.printf("   ESP32 3.3V → SHT31 VIN\n");
            Serial.printf("   ESP32 GND  → SHT31 GND\n");
            Serial.printf("   ESP32 GPIO%d → SHT31 SDA\n", SDA_PIN);
            Serial.printf("   ESP32 GPIO%d → SHT31 SCL\n", SCL_PIN);
            return;
        }
    }
    
    Serial.println("✅ SHT31 sensor found and initialized!");
    Serial.println("📊 Reading temperature and humidity every 2 seconds...");
    Serial.println("----------------------------------------");
}

// Function to update SHT31 readings
void updateSHT31() {
    if (millis() - last_reading >= READING_INTERVAL) {
        // Read temperature and humidity
        float temp = sht31.readTemperature();
        float humidity = sht31.readHumidity();
        
        // Check if readings are valid (not NaN)
        if (!isnan(temp) && !isnan(humidity)) {
            current_temp = temp;
            current_humidity = humidity;
            last_reading = millis();
            
            // Print readings
            Serial.printf("🌡️ Temperature: %.2f°C (%.2f°F)\n", 
                         current_temp, (current_temp * 9.0/5.0) + 32.0);
            Serial.printf("💧 Humidity: %.1f%%\n", current_humidity);
            
            // Print temperature status
            if (current_temp < 0) {
                Serial.println("❄️ Temp Status: Cold");
            } else if (current_temp < 20) {
                Serial.println("🌤️ Temp Status: Cool");
            } else if (current_temp < 30) {
                Serial.println("☀️ Temp Status: Normal");
            } else if (current_temp < 40) {
                Serial.println("🔥 Temp Status: Warm");
            } else {
                Serial.println("🌋 Temp Status: Hot");
            }
            
            // Print humidity status
            if (current_humidity < 30) {
                Serial.println("🏜️ Humidity Status: Dry");
            } else if (current_humidity < 50) {
                Serial.println("🌵 Humidity Status: Low");
            } else if (current_humidity < 70) {
                Serial.println("🌤️ Humidity Status: Normal");
            } else if (current_humidity < 85) {
                Serial.println("🌧️ Humidity Status: High");
            } else {
                Serial.println("🌊 Humidity Status: Very Humid");
            }
            
            Serial.println("----------------------------------------");
        } else {
            Serial.println("❌ Failed to read from SHT31 sensor!");
        }
    }
}

// Function to get current temperature (for external use)
float getCurrentTemperature() {
    return current_temp;
}

// Function to get current humidity (for external use)
float getCurrentHumidity() {
    return current_humidity;
}

// Function to check if temperature is in safe range
bool isTemperatureSafe() {
    return (current_temp >= -40.0 && current_temp <= 125.0);
}

// Function to check if humidity is in safe range
bool isHumiditySafe() {
    return (current_humidity >= 0.0 && current_humidity <= 100.0);
}

// Function to get temperature status string
const char* getTemperatureStatus() {
    if (current_temp < 0) return "Cold";
    else if (current_temp < 20) return "Cool";
    else if (current_temp < 30) return "Normal";
    else if (current_temp < 40) return "Warm";
    else return "Hot";
}

// Function to get humidity status string
const char* getHumidityStatus() {
    if (current_humidity < 30) return "Dry";
    else if (current_humidity < 50) return "Low";
    else if (current_humidity < 70) return "Normal";
    else if (current_humidity < 85) return "High";
    else return "Very Humid";
}

// Function to print sensor info
void printSensorInfo() {
    Serial.println("📋 SHT31 Sensor Information:");
    Serial.println("   Interface: I2C");
    Serial.printf("   I2C Pins: SDA=GPIO%d, SCL=GPIO%d\n", SDA_PIN, SCL_PIN);
    Serial.println("   Address: 0x44 (or 0x45)");
    Serial.println("   Temperature Range: -40°C to +125°C");
    Serial.println("   Humidity Range: 0% to 100%");
    Serial.println("   Temperature Accuracy: ±0.2°C");
    Serial.println("   Humidity Accuracy: ±2%");
    Serial.println("   Wiring:");
    Serial.printf("     ESP32 3.3V → SHT31 VIN\n");
    Serial.printf("     ESP32 GND  → SHT31 GND\n");
    Serial.printf("     ESP32 GPIO%d → SHT31 SDA\n", SDA_PIN);
    Serial.printf("     ESP32 GPIO%d → SHT31 SCL\n", SCL_PIN);
    Serial.println("----------------------------------------");
} 