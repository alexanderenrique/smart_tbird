/*
 * Smart Thunderbird AP Display - Main Application
 * ===============================================
 * 
 * PURPOSE:
 * This is the main application for the Smart Thunderbird project. It creates a WiFi Access Point
 * with a touchscreen display that shows sensor data received from connected devices.
 * 
 * ENVIRONMENT:
 * - Hardware: ESP32 with TFT touchscreen display (480x320 resolution)
 * - Platform: PlatformIO with Arduino framework
 * - Libraries: LVGL (GUI), TFT_eSPI (display), WiFi, WebServer
 * 
 * FUNCTIONALITY:
 * 1. Creates WiFi Access Point "SmartThunderbird" (password: 12345678)
 * 2. Runs web server on port 8080 to receive sensor data via HTTP POST
 * 3. Displays sensor data UI with:
 *    - Real-time temperature display from connected sensors
 *    - Real-time humidity display from connected sensors
 *    - Status indicators with color coding
 *    - IP address display
 * 4. Receives temperature and humidity data from SHT31 sensors via HTTP
 * 5. Updates display in real-time as sensor data arrives
 * 
 * CONNECTIONS:
 * - TFT Display: Uses TFT_eSPI library with custom pin configuration
 * - Touch Input: Integrated capacitive touch on display
 * - WiFi: Built-in ESP32 WiFi for AP mode
 * 
 * USAGE:
 * Upload this to the main ESP32 display unit. Other sensor devices will connect
 * to the WiFi AP and send data to this display.
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <WiFi.h>
#include <WebServer.h>

TFT_eSPI tft = TFT_eSPI();

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[480 * 4];

// WiFi AP Configuration
const char* ssid = "SmartThunderbird";
const char* password = "12345678";
const int port = 8080;

// Web server for receiving sensor data
WebServer server(port);

// Display variables
static lv_obj_t *temp_label = NULL;
static lv_obj_t *humidity_label = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *ip_label = NULL;

// Sensor data from sensors
static float received_temp = 0.0;
static float received_humidity = 0.0;
static String sensor_status = "No sensor connected";

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
  tft.pushColors((uint16_t *)&color_p->full, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1), true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
  uint16_t touchX, touchY;
  if (tft.getTouch(&touchX, &touchY, 100)) {
      data->state = LV_INDEV_STATE_PR;
      data->point.x = touchX;
      data->point.y = touchY;
      Serial.printf("👆 Touch: X=%d Y=%d\n", touchX, touchY);
  } else {
      data->state = LV_INDEV_STATE_REL;
  }
}

// Web server handlers
void handleRoot() {
    String html = "<html><body>";
    html += "<h1>Smart Thunderbird Sensor Hub</h1>";
    float temp_f = (received_temp * 9.0/5.0) + 32.0;  // Convert C to F
    html += "<p>Temperature: " + String(temp_f, 1) + "°F</p>";
    html += "<p>Humidity: " + String(received_humidity, 1) + "%</p>";
    html += "<p>Status: " + sensor_status + "</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleSensorData() {
    bool has_temp = server.hasArg("temp");
    bool has_humidity = server.hasArg("humidity");
    
    if (has_temp || has_humidity) {
        if (has_temp) {
            received_temp = server.arg("temp").toFloat();
        }
        if (has_humidity) {
            received_humidity = server.arg("humidity").toFloat();
        }
        sensor_status = "Connected";
        
        // Update temperature display
        if (temp_label) {
            static char temp_buf[32];
            float temp_f = (received_temp * 9.0/5.0) + 32.0;  // Convert C to F
            snprintf(temp_buf, sizeof(temp_buf), "%.1f°F", temp_f);
            lv_label_set_text(temp_label, temp_buf);
        }
        
        // Update humidity display
        if (humidity_label) {
            static char humidity_buf[32];
            snprintf(humidity_buf, sizeof(humidity_buf), "%.1f%%", received_humidity);
            lv_label_set_text(humidity_label, humidity_buf);
        }
        
        if (status_label) {
            static char status_buf[64];
            const char* temp_status = "Normal";
            // Convert temperature thresholds to Fahrenheit
            if (received_temp < 0) temp_status = "Cold";      // < 32°F
            else if (received_temp < 20) temp_status = "Cool"; // < 68°F
            else if (received_temp < 30) temp_status = "Normal"; // < 86°F
            else if (received_temp < 40) temp_status = "Warm"; // < 104°F
            else temp_status = "Hot";                          // >= 104°F
            
            snprintf(status_buf, sizeof(status_buf), "Status: %s", temp_status);
            lv_label_set_text(status_label, status_buf);
            
            // Change color based on temperature (using Celsius thresholds)
            if (received_temp > 30) {
                lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF0000), 0); // Red (>86°F)
            } else if (received_temp < 10) {
                lv_obj_set_style_text_color(status_label, lv_color_hex(0x0000FF), 0); // Blue (<50°F)
            } else {
                lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0); // Green (50-86°F)
            }
        }
        
        Serial.printf("📡 Received - Temp: %.2f°C (%.1f°F), Humidity: %.1f%%\n", 
                     received_temp, (received_temp * 9.0/5.0) + 32.0, received_humidity);
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing sensor data");
    }
}

void setupWiFiAP() {
    Serial.println("📡 Setting up WiFi Access Point...");
    
    // Configure WiFi AP with explicit channel and settings
    WiFi.softAP(ssid, password, 6);  // Channel 6 (2.437 GHz)
    
    // Disable DHCP server on the AP
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    
    // Wait for AP to start
    delay(1000);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.printf("🌐 AP IP address: %s\n", IP.toString().c_str());
    Serial.printf("📡 WiFi Channel: %d (2.437 GHz)\n", WiFi.channel());
    Serial.printf("📡 SSID: %s\n", ssid);
    Serial.println("⚠️  DHCP is DISABLED - All devices must use static IP addresses");
    Serial.println("   Main Display: 192.168.4.1");
    Serial.println("   Sensor Devices: 192.168.4.100+ (static IPs required)");
    
    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/sensor", HTTP_POST, handleSensorData);
    
    server.begin();
    Serial.println("✅ Web server started on port " + String(port));
}

void createUI() {
    // Title label
    lv_obj_t *title_label = lv_label_create(lv_scr_act());
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_label_set_text(title_label, "Smart Thunderbird");

    // IP Address label
    ip_label = lv_label_create(lv_scr_act());
    lv_obj_align(ip_label, LV_ALIGN_TOP_LEFT, 20, 60);
    lv_label_set_text(ip_label, "IP: 192.168.4.1");

    // Temperature label
    temp_label = lv_label_create(lv_scr_act());
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 20, 100);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(temp_label, "0.0°F");

    // Humidity label
    humidity_label = lv_label_create(lv_scr_act());
    lv_obj_align(humidity_label, LV_ALIGN_TOP_LEFT, 20, 140);
    lv_obj_set_style_text_font(humidity_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(humidity_label, "0.0%");

    // Status label
    status_label = lv_label_create(lv_scr_act());
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 20, 180);
    lv_label_set_text(status_label, "Status: Waiting for sensor...");
}

void setup() {
    Serial.begin(9600);
    Serial.println("🚀 Smart Thunderbird AP Display starting up...");

    // 1. Init display and LVGL
    tft.init();
    tft.setRotation(1);
    lv_init();

    // 2. Setup display buffer
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, sizeof(buf1) / sizeof(lv_color_t));

    // 3. Initialize and register display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 480;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_t *disp = lv_disp_drv_register(&disp_drv);

    // 4. Create UI
    createUI();

    // 5. Setup WiFi AP
    setupWiFiAP();

    // 6. Initialize and register input driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_t *my_indev = lv_indev_drv_register(&indev_drv);

    if (!my_indev) {
        Serial.println("🚨 LVGL: Failed to register input driver!");
    } else {
        Serial.println("✅ LVGL: Input driver registered successfully.");
    }

    Serial.println("🎯 Ready! Connect sensor to WiFi: " + String(ssid));
}

void loop() {
    lv_tick_inc(5);
    lv_timer_handler();
    
    // Handle web server requests
    server.handleClient();
    
    delay(5);
} 