/*
 * Display Node - Main Display Unit for Smart Thunderbird
 * =====================================================
 * 
 * This is the main display unit that shows sensor data from all nodes
 * in the CAN network. It features:
 * - TFT touchscreen display with LVGL
 * - Real-time sensor data visualization
 * - CAN network status monitoring
 * - Touch interface for user interaction
 */

#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>

// Include shared libraries
#include "../shared/can_lib/can_messages.h"
#include "../shared/can_lib/can_manager.h"
#include "../shared/utils/node_config.h"

// LVGL Configuration
#include "lv_conf.h"

// Display and touch objects
TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 10];

// CAN Manager
CANManager* can_manager = nullptr;

// Comprehensive sensor data storage
struct SHT31Data {
    float temperature = 0.0;
    float humidity = 0.0;
    bool temp_valid = false;
    bool humidity_valid = false;
    unsigned long last_update = 0;
    uint8_t sensor_id = 0;
};

struct TMP36Data {
    float temperature = 0.0;
    bool temp_valid = false;
    unsigned long last_update = 0;
    uint8_t sensor_id = 0;
};

struct MPU6050Data {
    float accel_x = 0.0;
    float accel_y = 0.0;
    float accel_z = 0.0;
    float max_accel_x = 0.0;
    float max_accel_y = 0.0;
    float max_accel_z = 0.0;
    bool accel_valid = false;
    bool max_valid = false;
    unsigned long last_update = 0;
    uint8_t sensor_id = 0;
};

struct INA219Data {
    float voltage = 0.0;
    float current = 0.0;
    float power = 0.0;
    bool voltage_valid = false;
    bool current_valid = false;
    bool power_valid = false;
    unsigned long last_update = 0;
    uint8_t sensor_id = 0;
};

struct SystemStatus {
    bool can_healthy = false;
    unsigned long last_can_message = 0;
    uint32_t message_count = 0;
    uint16_t free_memory = 0;
    uint8_t device_temp = 0;
    unsigned long uptime = 0;
};

struct LDRData {
    float light_level = 0.0;
    uint8_t brightness = 128;
    uint8_t target_brightness = 128;
    bool valid = false;
    unsigned long last_update = 0;
    
    // Averaging variables
    int readings[LDR_AVERAGE_SAMPLES];
    int index = 0;
    int sum = 0;
    bool array_filled = false;
    int averaged_value = 0;
};

// Global sensor data
SHT31Data sht31_data;
TMP36Data tmp36_data;
MPU6050Data mpu6050_data;
INA219Data ina219_data;
SystemStatus system_status;
LDRData ldr_data;

// UI Objects - Main containers
lv_obj_t* main_screen;
lv_obj_t* header_container;
lv_obj_t* content_container;
lv_obj_t* footer_container;

// Header UI Objects
lv_obj_t* title_label;
lv_obj_t* can_status_label;
lv_obj_t* system_status_label;

// Sensor card containers
lv_obj_t* sht31_card;
lv_obj_t* tmp36_card;
lv_obj_t* mpu6050_card;
lv_obj_t* ina219_card;

// SHT31 card labels
lv_obj_t* sht31_title;
lv_obj_t* sht31_temp_label;
lv_obj_t* sht31_humidity_label;
lv_obj_t* sht31_status_label;

// TMP36 card labels
lv_obj_t* tmp36_title;
lv_obj_t* tmp36_temp_label;
lv_obj_t* tmp36_status_label;

// MPU6050 card labels
lv_obj_t* mpu6050_title;
lv_obj_t* mpu6050_accel_label;
lv_obj_t* mpu6050_max_label;
lv_obj_t* mpu6050_status_label;

// INA219 card labels
lv_obj_t* ina219_title;
lv_obj_t* ina219_voltage_label;
lv_obj_t* ina219_current_label;
lv_obj_t* ina219_power_label;
lv_obj_t* ina219_status_label;

// Footer labels
lv_obj_t* uptime_label;
lv_obj_t* memory_label;
lv_obj_t* ldr_label;

// Timing variables
unsigned long last_sensor_update = 0;
unsigned long last_heartbeat = 0;
unsigned long last_can_check = 0;
unsigned long last_ldr_read = 0;

// ============================================================================
// LVGL DISPLAY DRIVER
// ============================================================================

void my_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t* indev_driver, lv_indev_data_t* data) {
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY);

    if (touched) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// ============================================================================
// UI SETUP
// ============================================================================

void setupUI() {
    // Create main screen
    main_screen = lv_obj_create(NULL);
    lv_scr_load(main_screen);
    
    // Set main screen style
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(main_screen, 0, 0);
    lv_obj_set_style_pad_all(main_screen, 0, 0);
    
    // ============================================================================
    // HEADER SECTION (Top 40px)
    // ============================================================================
    header_container = lv_obj_create(main_screen);
    lv_obj_set_size(header_container, 320, 40);
    lv_obj_set_pos(header_container, 0, 0);
    lv_obj_set_style_bg_color(header_container, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(header_container, 0, 0);
    lv_obj_set_style_pad_all(header_container, 5, 0);
    
    // Title
    title_label = lv_label_create(header_container);
    lv_label_set_text(title_label, "Smart Thunderbird");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x00D4FF), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 5, 0);
    
    // CAN Status
    can_status_label = lv_label_create(header_container);
    lv_label_set_text(can_status_label, "CAN: --");
    lv_obj_set_style_text_color(can_status_label, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_text_font(can_status_label, &lv_font_montserrat_12, 0);
    lv_obj_align(can_status_label, LV_ALIGN_RIGHT_MID, -5, 0);
    
    // ============================================================================
    // CONTENT SECTION (Middle 160px - 2x2 grid)
    // ============================================================================
    content_container = lv_obj_create(main_screen);
    lv_obj_set_size(content_container, 320, 160);
    lv_obj_set_pos(content_container, 0, 40);
    lv_obj_set_style_bg_color(content_container, lv_color_hex(0x0A0A0A), 0);
    lv_obj_set_style_border_width(content_container, 0, 0);
    lv_obj_set_style_pad_all(content_container, 5, 0);
    
    // SHT31 Card (Top Left)
    sht31_card = lv_obj_create(content_container);
    lv_obj_set_size(sht31_card, 150, 75);
    lv_obj_set_pos(sht31_card, 5, 5);
    lv_obj_set_style_bg_color(sht31_card, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_border_width(sht31_card, 1, 0);
    lv_obj_set_style_border_color(sht31_card, lv_color_hex(0x00D4FF), 0);
    lv_obj_set_style_pad_all(sht31_card, 5, 0);
    
    sht31_title = lv_label_create(sht31_card);
    lv_label_set_text(sht31_title, "SHT31");
    lv_obj_set_style_text_color(sht31_title, lv_color_hex(0x00D4FF), 0);
    lv_obj_set_style_text_font(sht31_title, &lv_font_montserrat_12, 0);
    lv_obj_align(sht31_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    sht31_temp_label = lv_label_create(sht31_card);
    lv_label_set_text(sht31_temp_label, "Temp: --°C");
    lv_obj_set_style_text_color(sht31_temp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(sht31_temp_label, &lv_font_montserrat_10, 0);
    lv_obj_align(sht31_temp_label, LV_ALIGN_TOP_LEFT, 0, 15);
    
    sht31_humidity_label = lv_label_create(sht31_card);
    lv_label_set_text(sht31_humidity_label, "Humidity: --%");
    lv_obj_set_style_text_color(sht31_humidity_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(sht31_humidity_label, &lv_font_montserrat_10, 0);
    lv_obj_align(sht31_humidity_label, LV_ALIGN_TOP_LEFT, 0, 30);
    
    sht31_status_label = lv_label_create(sht31_card);
    lv_label_set_text(sht31_status_label, "Status: --");
    lv_obj_set_style_text_color(sht31_status_label, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_text_font(sht31_status_label, &lv_font_montserrat_8, 0);
    lv_obj_align(sht31_status_label, LV_ALIGN_TOP_LEFT, 0, 45);
    
    // TMP36 Card (Top Right)
    tmp36_card = lv_obj_create(content_container);
    lv_obj_set_size(tmp36_card, 150, 75);
    lv_obj_set_pos(tmp36_card, 165, 5);
    lv_obj_set_style_bg_color(tmp36_card, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_border_width(tmp36_card, 1, 0);
    lv_obj_set_style_border_color(tmp36_card, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_pad_all(tmp36_card, 5, 0);
    
    tmp36_title = lv_label_create(tmp36_card);
    lv_label_set_text(tmp36_title, "TMP36");
    lv_obj_set_style_text_color(tmp36_title, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_text_font(tmp36_title, &lv_font_montserrat_12, 0);
    lv_obj_align(tmp36_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    tmp36_temp_label = lv_label_create(tmp36_card);
    lv_label_set_text(tmp36_temp_label, "Temp: --°C");
    lv_obj_set_style_text_color(tmp36_temp_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(tmp36_temp_label, &lv_font_montserrat_10, 0);
    lv_obj_align(tmp36_temp_label, LV_ALIGN_TOP_LEFT, 0, 15);
    
    tmp36_status_label = lv_label_create(tmp36_card);
    lv_label_set_text(tmp36_status_label, "Status: --");
    lv_obj_set_style_text_color(tmp36_status_label, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_text_font(tmp36_status_label, &lv_font_montserrat_8, 0);
    lv_obj_align(tmp36_status_label, LV_ALIGN_TOP_LEFT, 0, 30);
    
    // MPU6050 Card (Bottom Left)
    mpu6050_card = lv_obj_create(content_container);
    lv_obj_set_size(mpu6050_card, 150, 75);
    lv_obj_set_pos(mpu6050_card, 5, 80);
    lv_obj_set_style_bg_color(mpu6050_card, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_border_width(mpu6050_card, 1, 0);
    lv_obj_set_style_border_color(mpu6050_card, lv_color_hex(0x4ECDC4), 0);
    lv_obj_set_style_pad_all(mpu6050_card, 5, 0);
    
    mpu6050_title = lv_label_create(mpu6050_card);
    lv_label_set_text(mpu6050_title, "MPU6050");
    lv_obj_set_style_text_color(mpu6050_title, lv_color_hex(0x4ECDC4), 0);
    lv_obj_set_style_text_font(mpu6050_title, &lv_font_montserrat_12, 0);
    lv_obj_align(mpu6050_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    mpu6050_accel_label = lv_label_create(mpu6050_card);
    lv_label_set_text(mpu6050_accel_label, "Accel: --g");
    lv_obj_set_style_text_color(mpu6050_accel_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(mpu6050_accel_label, &lv_font_montserrat_10, 0);
    lv_obj_align(mpu6050_accel_label, LV_ALIGN_TOP_LEFT, 0, 15);
    
    mpu6050_max_label = lv_label_create(mpu6050_card);
    lv_label_set_text(mpu6050_max_label, "Max: --g");
    lv_obj_set_style_text_color(mpu6050_max_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(mpu6050_max_label, &lv_font_montserrat_10, 0);
    lv_obj_align(mpu6050_max_label, LV_ALIGN_TOP_LEFT, 0, 30);
    
    mpu6050_status_label = lv_label_create(mpu6050_card);
    lv_label_set_text(mpu6050_status_label, "Status: --");
    lv_obj_set_style_text_color(mpu6050_status_label, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_text_font(mpu6050_status_label, &lv_font_montserrat_8, 0);
    lv_obj_align(mpu6050_status_label, LV_ALIGN_TOP_LEFT, 0, 45);
    
    // INA219 Card (Bottom Right)
    ina219_card = lv_obj_create(content_container);
    lv_obj_set_size(ina219_card, 150, 75);
    lv_obj_set_pos(ina219_card, 165, 80);
    lv_obj_set_style_bg_color(ina219_card, lv_color_hex(0x16213E), 0);
    lv_obj_set_style_border_width(ina219_card, 1, 0);
    lv_obj_set_style_border_color(ina219_card, lv_color_hex(0xFFE66D), 0);
    lv_obj_set_style_pad_all(ina219_card, 5, 0);
    
    ina219_title = lv_label_create(ina219_card);
    lv_label_set_text(ina219_title, "INA219");
    lv_obj_set_style_text_color(ina219_title, lv_color_hex(0xFFE66D), 0);
    lv_obj_set_style_text_font(ina219_title, &lv_font_montserrat_12, 0);
    lv_obj_align(ina219_title, LV_ALIGN_TOP_LEFT, 0, 0);
    
    ina219_voltage_label = lv_label_create(ina219_card);
    lv_label_set_text(ina219_voltage_label, "Voltage: --V");
    lv_obj_set_style_text_color(ina219_voltage_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ina219_voltage_label, &lv_font_montserrat_10, 0);
    lv_obj_align(ina219_voltage_label, LV_ALIGN_TOP_LEFT, 0, 15);
    
    ina219_current_label = lv_label_create(ina219_card);
    lv_label_set_text(ina219_current_label, "Current: --mA");
    lv_obj_set_style_text_color(ina219_current_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ina219_current_label, &lv_font_montserrat_10, 0);
    lv_obj_align(ina219_current_label, LV_ALIGN_TOP_LEFT, 0, 30);
    
    ina219_power_label = lv_label_create(ina219_card);
    lv_label_set_text(ina219_power_label, "Power: --W");
    lv_obj_set_style_text_color(ina219_power_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ina219_power_label, &lv_font_montserrat_10, 0);
    lv_obj_align(ina219_power_label, LV_ALIGN_TOP_LEFT, 0, 45);
    
    ina219_status_label = lv_label_create(ina219_card);
    lv_label_set_text(ina219_status_label, "Status: --");
    lv_obj_set_style_text_color(ina219_status_label, lv_color_hex(0xFF6B6B), 0);
    lv_obj_set_style_text_font(ina219_status_label, &lv_font_montserrat_8, 0);
    lv_obj_align(ina219_status_label, LV_ALIGN_TOP_LEFT, 0, 60);
    
    // ============================================================================
    // FOOTER SECTION (Bottom 40px)
    // ============================================================================
    footer_container = lv_obj_create(main_screen);
    lv_obj_set_size(footer_container, 320, 40);
    lv_obj_set_pos(footer_container, 0, 200);
    lv_obj_set_style_bg_color(footer_container, lv_color_hex(0x1A1A2E), 0);
    lv_obj_set_style_border_width(footer_container, 0, 0);
    lv_obj_set_style_pad_all(footer_container, 5, 0);
    
    // Uptime
    uptime_label = lv_label_create(footer_container);
    lv_label_set_text(uptime_label, "Uptime: --");
    lv_obj_set_style_text_color(uptime_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(uptime_label, &lv_font_montserrat_10, 0);
    lv_obj_align(uptime_label, LV_ALIGN_LEFT_MID, 5, 0);
    
    // Memory
    memory_label = lv_label_create(footer_container);
    lv_label_set_text(memory_label, "RAM: --KB");
    lv_obj_set_style_text_color(memory_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(memory_label, &lv_font_montserrat_10, 0);
    lv_obj_align(memory_label, LV_ALIGN_CENTER, 0, 0);
    
    // LDR
    ldr_label = lv_label_create(footer_container);
    lv_label_set_text(ldr_label, "Light: --");
    lv_obj_set_style_text_color(ldr_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(ldr_label, &lv_font_montserrat_10, 0);
    lv_obj_align(ldr_label, LV_ALIGN_RIGHT_MID, -5, 0);
}

// ============================================================================
// CAN MESSAGE HANDLERS
// ============================================================================

void handleSHT31Message(const SHT31Data& data) {
    sht31_data = data;
    sht31_data.last_update = millis();
    
    // Update UI
    char temp_str[20];
    char humidity_str[20];
    char status_str[20];
    
    snprintf(temp_str, sizeof(temp_str), "Temp: %.1f°C", sht31_data.temperature);
    snprintf(humidity_str, sizeof(humidity_str), "Humidity: %.1f%%", sht31_data.humidity);
    snprintf(status_str, sizeof(status_str), "Status: %s", 
             (sht31_data.temp_valid && sht31_data.humidity_valid) ? "OK" : "ERROR");
    
    lv_label_set_text(sht31_temp_label, temp_str);
    lv_label_set_text(sht31_humidity_label, humidity_str);
    lv_label_set_text(sht31_status_label, status_str);
    
    // Update status color
    lv_obj_set_style_text_color(sht31_status_label, 
        (sht31_data.temp_valid && sht31_data.humidity_valid) ? 
        lv_color_hex(0x4ECDC4) : lv_color_hex(0xFF6B6B), 0);
}

void handleTMP36Message(const TMP36Data& data) {
    tmp36_data = data;
    tmp36_data.last_update = millis();
    
    // Update UI
    char temp_str[20];
    char status_str[20];
    
    snprintf(temp_str, sizeof(temp_str), "Temp: %.1f°C", tmp36_data.temperature);
    snprintf(status_str, sizeof(status_str), "Status: %s", 
             tmp36_data.temp_valid ? "OK" : "ERROR");
    
    lv_label_set_text(tmp36_temp_label, temp_str);
    lv_label_set_text(tmp36_status_label, status_str);
    
    // Update status color
    lv_obj_set_style_text_color(tmp36_status_label, 
        tmp36_data.temp_valid ? lv_color_hex(0x4ECDC4) : lv_color_hex(0xFF6B6B), 0);
}

void handleMPU6050Message(const MPU6050Data& data) {
    mpu6050_data = data;
    mpu6050_data.last_update = millis();
    
    // Update UI
    char accel_str[30];
    char max_str[30];
    char status_str[20];
    
    snprintf(accel_str, sizeof(accel_str), "Accel: %.2f,%.2f,%.2f", 
             mpu6050_data.accel_x, mpu6050_data.accel_y, mpu6050_data.accel_z);
    snprintf(max_str, sizeof(max_str), "Max: %.2f,%.2f,%.2f", 
             mpu6050_data.max_accel_x, mpu6050_data.max_accel_y, mpu6050_data.max_accel_z);
    snprintf(status_str, sizeof(status_str), "Status: %s", 
             mpu6050_data.accel_valid ? "OK" : "ERROR");
    
    lv_label_set_text(mpu6050_accel_label, accel_str);
    lv_label_set_text(mpu6050_max_label, max_str);
    lv_label_set_text(mpu6050_status_label, status_str);
    
    // Update status color
    lv_obj_set_style_text_color(mpu6050_status_label, 
        mpu6050_data.accel_valid ? lv_color_hex(0x4ECDC4) : lv_color_hex(0xFF6B6B), 0);
}

void handleINA219Message(const INA219Data& data) {
    ina219_data = data;
    ina219_data.last_update = millis();
    
    // Update UI
    char voltage_str[20];
    char current_str[20];
    char power_str[20];
    char status_str[20];
    
    snprintf(voltage_str, sizeof(voltage_str), "Voltage: %.2fV", ina219_data.voltage);
    snprintf(current_str, sizeof(current_str), "Current: %.1fmA", ina219_data.current);
    snprintf(power_str, sizeof(power_str), "Power: %.2fW", ina219_data.power);
    snprintf(status_str, sizeof(status_str), "Status: %s", 
             (ina219_data.voltage_valid && ina219_data.current_valid) ? "OK" : "ERROR");
    
    lv_label_set_text(ina219_voltage_label, voltage_str);
    lv_label_set_text(ina219_current_label, current_str);
    lv_label_set_text(ina219_power_label, power_str);
    lv_label_set_text(ina219_status_label, status_str);
    
    // Update status color
    lv_obj_set_style_text_color(ina219_status_label, 
        (ina219_data.voltage_valid && ina219_data.current_valid) ? 
        lv_color_hex(0x4ECDC4) : lv_color_hex(0xFF6B6B), 0);
}

void handleHeartbeatMessage(const Heartbeat& data) {
    system_status.uptime = data.uptime_seconds;
    system_status.free_memory = data.free_memory;
    system_status.device_temp = data.temperature;
    
    // Update footer
    char uptime_str[20];
    char memory_str[20];
    
    snprintf(uptime_str, sizeof(uptime_str), "Uptime: %lus", system_status.uptime);
    snprintf(memory_str, sizeof(memory_str), "RAM: %uKB", system_status.free_memory / 1024);
    
    lv_label_set_text(uptime_label, uptime_str);
    lv_label_set_text(memory_label, memory_str);
}

// ============================================================================
// LDR AUTO-DIMMING FUNCTIONS
// ============================================================================

int applyGammaCorrection(float normalizedBrightness) {
    // Apply gamma correction for human perception
    float gammaCorrected = pow(normalizedBrightness, 1.0f / LDR_GAMMA);
    
    // Convert back to 0-255 range
    int result = (int)(gammaCorrected * (LDR_BRIGHTNESS_MAX - LDR_BRIGHTNESS_MIN) + LDR_BRIGHTNESS_MIN);
    
    // Ensure we stay within bounds
    return constrain(result, LDR_BRIGHTNESS_MIN, LDR_BRIGHTNESS_MAX);
}

void updateLDRData(int ldrValue) {
    // Add to averaging array
    ldr_data.readings[ldr_data.index] = ldrValue;
    ldr_data.sum += ldrValue;
    ldr_data.index++;
    
    if (ldr_data.index >= LDR_AVERAGE_SAMPLES) {
        ldr_data.index = 0;
        ldr_data.array_filled = true;
    }
    
    // Calculate average
    int sampleCount = ldr_data.array_filled ? LDR_AVERAGE_SAMPLES : ldr_data.index;
    ldr_data.averaged_value = ldr_data.sum / sampleCount;
}

uint8_t calculateBrightness(int ldrValue) {
    uint8_t targetBrightness = LDR_BRIGHTNESS_MIN;
    
    if (ldrValue < LDR_DARK_THRESHOLD) {
        targetBrightness = LDR_BRIGHTNESS_MIN;
    } else if (ldrValue > LDR_BRIGHT_THRESHOLD) {
        targetBrightness = LDR_BRIGHTNESS_MAX;
    } else {
        // Linear interpolation between thresholds
        float normalizedBrightness = (float)(ldrValue - LDR_DARK_THRESHOLD) / 
                                    (float)(LDR_BRIGHT_THRESHOLD - LDR_DARK_THRESHOLD);
        targetBrightness = applyGammaCorrection(normalizedBrightness);
    }
    
    return targetBrightness;
}

void updateDisplayBrightness(uint8_t brightness) {
    // Control TFT backlight brightness via PWM with high frequency
    ledcWrite(0, brightness);
}

void readLDRData() {
    static unsigned long last_debug = 0;
    
    if (millis() - last_ldr_read < LDR_READ_INTERVAL) {
        return;
    }
    last_ldr_read = millis();
    
    // Read LDR value
    int ldrValue = analogRead(LDR_ANALOG_PIN);
    ldr_data.light_level = (float)ldrValue;
    ldr_data.valid = true;
    ldr_data.last_update = millis();
    
    // Update averaging
    updateLDRData(ldrValue);
    
    // Calculate target brightness
    ldr_data.target_brightness = calculateBrightness(ldr_data.averaged_value);
    
    // Smooth brightness changes
    if (ldr_data.target_brightness > ldr_data.brightness) {
        ldr_data.brightness += LDR_SMOOTHING_FACTOR;
        if (ldr_data.brightness > ldr_data.target_brightness) {
            ldr_data.brightness = ldr_data.target_brightness;
        }
    } else if (ldr_data.target_brightness < ldr_data.brightness) {
        ldr_data.brightness -= LDR_SMOOTHING_FACTOR;
        if (ldr_data.brightness < ldr_data.target_brightness) {
            ldr_data.brightness = ldr_data.target_brightness;
        }
    }
    
    // Update display brightness
    updateDisplayBrightness(ldr_data.brightness);
    
    // Update UI
    char ldr_str[20];
    snprintf(ldr_str, sizeof(ldr_str), "Light: %d", ldr_data.averaged_value);
    lv_label_set_text(ldr_label, ldr_str);
    
    // Debug output
    if (millis() - last_debug >= 10000) { // Every 10 seconds
        Serial.printf("LDR: %d (avg: %d) | Brightness: %d (target: %d)\n", 
                     ldrValue, ldr_data.averaged_value, ldr_data.brightness, ldr_data.target_brightness);
        last_debug = millis();
    }
}

// ============================================================================
// CAN MESSAGE PROCESSING
// ============================================================================

void processCANMessages() {
    if (!can_manager) return;
    
    // Process message queue
    can_manager->processQueue();
    
    // Update CAN status
    if (can_manager->isConnected()) {
        system_status.can_healthy = true;
        system_status.message_count = can_manager->getMessageCount();
        lv_label_set_text(can_status_label, "CAN: OK");
        lv_obj_set_style_text_color(can_status_label, lv_color_hex(0x4ECDC4), 0);
    } else {
        system_status.can_healthy = false;
        lv_label_set_text(can_status_label, "CAN: ERROR");
        lv_obj_set_style_text_color(can_status_label, lv_color_hex(0xFF6B6B), 0);
    }
}

// ============================================================================
// MAIN SETUP
// ============================================================================

void setup() {
    Serial.begin(9600);
    delay(1000);
    
    Serial.println("=== Smart Thunderbird Display Node ===");
    
    // Initialize LDR pin for auto-dimming
    pinMode(LDR_ANALOG_PIN, INPUT);
    Serial.printf("LDR initialized on pin %d\n", LDR_ANALOG_PIN);
    
    // Initialize display
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    
    // Initialize backlight control with high-frequency PWM
    pinMode(TFT_BL_PIN, OUTPUT);
    ledcSetup(0, LDR_PWM_FREQUENCY, LDR_PWM_RESOLUTION);  // Channel 0, 25kHz, 8-bit resolution
    ledcAttachPin(TFT_BL_PIN, 0);
    ledcWrite(0, 128); // Start at medium brightness
    
    // Initialize LVGL
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 10);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    // Initialize touch
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    
    // Setup UI
    setupUI();
    
    // Initialize CAN manager
    can_manager = new CANManager(NODE_ID, NODE_TYPE);
    if (!can_manager->begin()) {
        Serial.println("ERROR: Failed to initialize CAN manager");
        lv_label_set_text(can_status_label, "CAN: ERROR");
        return;
    }
    
    Serial.println("Display node initialized successfully");
    lv_label_set_text(can_status_label, "CAN: OK");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    unsigned long current_time = millis();
    
    // Handle LVGL tasks
    lv_timer_handler();
    
    // Read LDR data for auto-dimming
    readLDRData();
    
    // Process CAN messages
    processCANMessages();
    
    delay(10);
}
