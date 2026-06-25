/*
 * Smart Thunderbird CAN Display - Main Application
 * ================================================
 * 
 * PURPOSE:
 * This is the main application for the Smart Thunderbird project. It receives sensor data
 * via CAN-Bus and displays it on a touchscreen display.
 * 
 * ENVIRONMENT:
 * - Hardware: ESP32 with TFT touchscreen display (480x320 resolution) + MCP2515 CAN controller
 * - Platform: PlatformIO with Arduino framework
 * - Libraries: LVGL (GUI), TFT_eSPI (display), MCP2515 (CAN)
 * 
 * FUNCTIONALITY:
 * 1. Receives sensor data via CAN-Bus from connected sensor devices
 * 2. Displays sensor data UI with:
 *    - Real-time temperature display from connected sensors
 *    - Real-time humidity display from connected sensors
 *    - Status indicators with color coding
 *    - CAN-Bus status display
 * 3. Receives temperature and humidity data from SHT31 and TMP36 sensors via CAN
 * 4. Updates display in real-time as sensor data arrives
 * 5. Monitors CAN-Bus health and sensor connectivity
 * 
 * CONNECTIONS:
 * - TFT Display: Uses TFT_eSPI library with custom pin configuration
 * - Touch Input: Integrated capacitive touch on display
 * - CAN-Bus: MCP2515 controller with TJA1050 transceiver
 * 
 * USAGE:
 * Upload this to the main ESP32 display unit. Other sensor devices will send data
 * via CAN-Bus to this display.
 */

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <SPI.h>
#include <Adafruit_MCP2515.h>
#include "can_messages.h"

TFT_eSPI tft = TFT_eSPI();

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[480 * 4];

// CAN-Bus Configuration
Adafruit_MCP2515 can(CAN_CS_PIN, CAN_MOSI_PIN, CAN_MISO_PIN, CAN_CLK_PIN);

// CAN message reception tracking
static unsigned long last_can_message = 0;
static unsigned long can_message_count = 0;
static bool can_bus_healthy = false;

// Display variables
static lv_obj_t *temp_label = NULL;
static lv_obj_t *humidity_label = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *ip_label = NULL;

// Sensor data from sensors
static float received_temp = 0.0;
static float received_humidity = 0.0;
static String sensor_status = "No sensor connected";
static uint8_t connected_sensors = 0;
static uint8_t sht31_sensor_id = 0;
static uint8_t tmp36_sensor_id = 0;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
  tft.pushColors((uint16_t *)&color_p->full, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1), true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
  // Touch functionality disabled for now - TFT_eSPI touch not configured
  // TODO: Configure TFT_eSPI touch pins or implement alternative touch method
  data->state = LV_INDEV_STATE_REL;
}

// CAN message handlers
void handleSHT31Data() {
    if (can.parsePacket()) {
        long id = can.packetId();
        int dlc = can.packetDlc();
        
        if (dlc == sizeof(SHT31Data)) {
            SHT31Data data;
            // Read data from CAN buffer
            for (int i = 0; i < dlc; i++) {
                ((uint8_t*)&data)[i] = can.read();
            }
            
            // Validate checksum
            if (validateChecksum((uint8_t*)&data, dlc)) {
                received_temp = rawToTemperature(data.temperature_raw);
                received_humidity = rawToHumidity(data.humidity_raw);
                sht31_sensor_id = data.sensor_id;
                sensor_status = "SHT31 Connected";
                connected_sensors |= 0x01; // Set bit 0 for SHT31
                
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
                
                // Update status display
                if (status_label) {
                    static char status_buf[64];
                    const char* temp_status = "Normal";
                    if (received_temp < 0) temp_status = "Cold";      // < 32°F
                    else if (received_temp < 20) temp_status = "Cool"; // < 68°F
                    else if (received_temp < 30) temp_status = "Normal"; // < 86°F
                    else if (received_temp < 40) temp_status = "Warm"; // < 104°F
                    else temp_status = "Hot";                          // >= 104°F
                    
                    snprintf(status_buf, sizeof(status_buf), "Status: %s", temp_status);
                    lv_label_set_text(status_label, status_buf);
                    
                    // Change color based on temperature
                    if (received_temp > 30) {
                        lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF0000), 0); // Red (>86°F)
                    } else if (received_temp < 10) {
                        lv_obj_set_style_text_color(status_label, lv_color_hex(0x0000FF), 0); // Blue (<50°F)
                    } else {
                        lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0); // Green (50-86°F)
                    }
                }
                
                Serial.printf("📡 CAN SHT31 - Temp: %.2f°C (%.1f°F), Humidity: %.1f%%, Sensor ID: %d\n", 
                             received_temp, (received_temp * 9.0/5.0) + 32.0, received_humidity, sht31_sensor_id);
            } else {
                Serial.println("❌ SHT31 data checksum validation failed!");
            }
        }
    }
}

void handleTMP36Data() {
    if (can.parsePacket()) {
        long id = can.packetId();
        int dlc = can.packetDlc();
        
        if (dlc == sizeof(TMP36Data)) {
            TMP36Data data;
            // Read data from CAN buffer
            for (int i = 0; i < dlc; i++) {
                ((uint8_t*)&data)[i] = can.read();
            }
            
            if (data.status_flags & 0x01) { // Temperature valid flag
                received_temp = rawToTemperature(data.temperature_raw);
                tmp36_sensor_id = data.sensor_id;
                sensor_status = "TMP36 Connected";
                connected_sensors |= 0x02; // Set bit 1 for TMP36
                
                // Update temperature display
                if (temp_label) {
                    static char temp_buf[32];
                    float temp_f = (received_temp * 9.0/5.0) + 32.0;  // Convert C to F
                    snprintf(temp_buf, sizeof(temp_buf), "%.1f°F", temp_f);
                    lv_label_set_text(temp_label, temp_buf);
                }
                
                Serial.printf("📡 CAN TMP36 - Temp: %.2f°C (%.1f°F), Sensor ID: %d\n", 
                             received_temp, (received_temp * 9.0/5.0) + 32.0, tmp36_sensor_id);
            }
        }
    }
}

void handleSensorStatus() {
    if (can.parsePacket()) {
        long id = can.packetId();
        int dlc = can.packetDlc();
        
        if (dlc == sizeof(SensorStatus)) {
            SensorStatus status;
            // Read data from CAN buffer
            for (int i = 0; i < dlc; i++) {
                ((uint8_t*)&status)[i] = can.read();
            }
            Serial.printf("📊 Sensor Status - Type: %d, ID: %d, Health: %d, Battery: %d%%, Uptime: %ds, Errors: %d\n",
                         status.sensor_type, status.sensor_id, status.health_status, 
                         status.battery_level, status.uptime_seconds, status.error_count);
        }
    }
}

void handleHeartbeat() {
    if (can.parsePacket()) {
        long id = can.packetId();
        int dlc = can.packetDlc();
        
        if (dlc == sizeof(Heartbeat)) {
            Heartbeat hb;
            // Read data from CAN buffer
            for (int i = 0; i < dlc; i++) {
                ((uint8_t*)&hb)[i] = can.read();
            }
            Serial.printf("💓 Heartbeat - Device: %d, ID: %d, Uptime: %ds, Memory: %d bytes, Temp: %d°C\n",
                         hb.device_type, hb.device_id, hb.uptime_seconds, hb.free_memory, hb.temperature);
        }
    }
}

void setupCAN() {
    Serial.println("🚌 Setting up CAN-Bus...");
    
    // Initialize SPI for MCP2515
    SPI.begin(CAN_CLK_PIN, CAN_MISO_PIN, CAN_MOSI_PIN, CAN_CS_PIN);
    
    // Initialize MCP2515 CAN controller
    if (!can.begin(500000)) { // 500 kbps
        Serial.println("❌ Failed to initialize CAN controller!");
        Serial.println("   Check MCP2515 connections and SPI pins");
        Serial.printf("   CS: GPIO%d, INT: GPIO%d, CLK: GPIO%d, MOSI: GPIO%d, MISO: GPIO%d\n",
                     CAN_CS_PIN, CAN_INT_PIN, CAN_CLK_PIN, CAN_MOSI_PIN, CAN_MISO_PIN);
        return;
    }
    
    Serial.println("✅ CAN-Bus initialized successfully!");
    Serial.printf("🚌 CAN Speed: 500 kbps\n");
    Serial.printf("🚌 CAN Pins - CS: GPIO%d, INT: GPIO%d, CLK: GPIO%d, MOSI: GPIO%d, MISO: GPIO%d\n",
                 CAN_CS_PIN, CAN_INT_PIN, CAN_CLK_PIN, CAN_MOSI_PIN, CAN_MISO_PIN);
    Serial.println("🚌 Waiting for sensor data...");
    
    can_bus_healthy = true;
}

void createUI() {
    // Title label
    lv_obj_t *title_label = lv_label_create(lv_scr_act());
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_label_set_text(title_label, "Smart Thunderbird");

    // CAN Status label
    ip_label = lv_label_create(lv_scr_act());
    lv_obj_align(ip_label, LV_ALIGN_TOP_LEFT, 20, 60);
    lv_label_set_text(ip_label, "CAN: Initializing...");

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

    // 5. Setup CAN-Bus
    setupCAN();

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

    Serial.println("🎯 Ready! Waiting for sensor data via CAN-Bus...");
}

void loop() {
    lv_tick_inc(5);
    lv_timer_handler();
    
    // Handle CAN messages
    if (can.parsePacket()) {
        long id = can.packetId();
        last_can_message = millis();
        can_message_count++;
        
        // Update CAN status display
        if (ip_label) {
            static char can_status[64];
            snprintf(can_status, sizeof(can_status), "CAN: Active (Msg: %lu)", can_message_count);
            lv_label_set_text(ip_label, can_status);
        }
        
        // Route messages to appropriate handlers based on ID
        switch (id) {
            case CAN_MSG_SHT31_TEMP_HUMIDITY:
                handleSHT31Data();
                break;
            case CAN_MSG_TMP36_TEMPERATURE:
                handleTMP36Data();
                break;
            case CAN_MSG_SENSOR_STATUS:
                handleSensorStatus();
                break;
            case CAN_MSG_HEARTBEAT:
                handleHeartbeat();
                break;
            default:
                Serial.printf("❓ Unknown CAN ID: 0x%03X\n", id);
                break;
        }
    }
    
    // Update CAN status if no messages received
    if (millis() - last_can_message > 10000 && can_bus_healthy) { // 10 seconds
        if (ip_label) {
            lv_label_set_text(ip_label, "CAN: No Data");
        }
    }
    
    delay(5);
} 