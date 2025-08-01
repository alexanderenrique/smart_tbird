#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>

// LED Configuration for ESP32-C3
#define LED_PIN 8  // Built-in LED on ESP32-C3

// I2C Configuration for SHT31
#define SHT31_ADDR 0x44  // SHT31 I2C address
#define SDA_PIN 6        // I2C Data pin (different from LED pin)
#define SCL_PIN 7        // I2C Clock pin

// WiFi Configuration
const char* ssid = "SmartThunderbird";
const char* password = "12345678";
const char* server_url = "http://192.168.4.1:8080/sensor";

// Static IP Configuration
IPAddress staticIP(192, 168, 4, 100);    // Static IP for this device
IPAddress gateway(192, 168, 4, 1);       // Gateway (usually the AP IP)
IPAddress subnet(255, 255, 255, 0);      // Subnet mask
IPAddress dns(192, 168, 4, 1);           // DNS server (same as gateway)

// WiFi client
WiFiClient client;
HTTPClient http;

// Timing variables
static unsigned long last_send = 0;
static const unsigned long SEND_INTERVAL = 5000; // Send every 5 seconds

// Temperature and humidity variables
static float current_temp = 0.0;
static float current_humidity = 0.0;
static unsigned long last_reading = 0;
static const unsigned long READING_INTERVAL = 2000; // Read every 2 seconds

// Function to initialize SHT31 sensor
void initSHT31() {
    // Initialize I2C communication
    Wire.begin(SDA_PIN, SCL_PIN);
    
    // Try to communicate with SHT31
    Wire.beginTransmission(SHT31_ADDR);
    Wire.write(0x27);  // Soft reset command
    Wire.endTransmission();
    delay(10);
}

// Function to read temperature and humidity from SHT31
bool readSHT31(float &temperature, float &humidity) {
    // Send measurement command
    Wire.beginTransmission(SHT31_ADDR);
    Wire.write(0x2C);  // Measurement command
    Wire.write(0x06);  // High repeatability
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    delay(15);  // Wait for measurement
    
    // Request data
    Wire.requestFrom(SHT31_ADDR, 6);
    if (Wire.available() != 6) {
        return false;
    }
    
    // Read temperature data
    uint16_t temp_raw = Wire.read() << 8;
    temp_raw |= Wire.read();
    Wire.read(); // CRC (ignore for now)
    
    // Read humidity data
    uint16_t hum_raw = Wire.read() << 8;
    hum_raw |= Wire.read();
    Wire.read(); // CRC (ignore for now)
    
    // Convert to actual values
    temperature = -45.0 + 175.0 * (temp_raw / 65535.0);
    humidity = 100.0 * (hum_raw / 65535.0);
    
    return true;
}

void setupWiFi() {
    // Start with LED off (HIGH for active LOW LED)
    digitalWrite(LED_PIN, HIGH);
    
    // Configure static IP
    WiFi.config(staticIP, gateway, subnet, dns);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        // Flash LED while connecting (fast flash)
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(100);  // Flash every 100ms
        attempts++;
    }
}

void sendSensorData(float temperature, float humidity) {
    if (WiFi.status() != WL_CONNECTED) {
        return;  // Don't send if not connected
    }
    
    http.begin(client, server_url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    // Send both temperature and humidity data
    String post_data = "temp=" + String(temperature, 2) + "&humidity=" + String(humidity, 1);
    
    int httpResponseCode = http.POST(post_data);
    http.end();
}

void setup() {
    // Initialize LED pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // Start with LED off (active LOW)
    
    // Initialize the SHT31 sensor
    initSHT31();
    
    // Setup WiFi connection
    setupWiFi();
}

void loop() {
    // Update SHT31 readings
    if (millis() - last_reading >= READING_INTERVAL) {
        float temp, humidity;
        if (readSHT31(temp, humidity)) {
            current_temp = temp;
            current_humidity = humidity;
            last_reading = millis();
        }
    }
    
    // Send sensor data every 5 seconds
    if (millis() - last_send >= SEND_INTERVAL) {
        sendSensorData(current_temp, current_humidity);
        last_send = millis();
    }
    
    // LED status based on WiFi connection
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