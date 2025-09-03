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
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x000000), 0);
    lv_scr_load(main_screen);

    // Temperature label
    temp_label = lv_label_create(main_screen);
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(temp_label, "Temperature: --°C");
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 20, 20);

    // Humidity label
    humidity_label = lv_label_create(main_screen);
    lv_obj_set_style_text_color(humidity_label, lv_color_hex(0x0080FF), 0);
    lv_obj_set_style_text_font(humidity_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(humidity_label, "Humidity: --%");
    lv_obj_align(humidity_label, LV_ALIGN_TOP_LEFT, 20, 60);

    // LDR label
    ldr_label = lv_label_create(main_screen);
    lv_obj_set_style_text_color(ldr_label, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_text_font(ldr_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(ldr_label, "Light: --%");
    lv_obj_align(ldr_label, LV_ALIGN_TOP_LEFT, 20, 100);

    // Status label
    status_label = lv_label_create(main_screen);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(status_label, "Status: Initializing...");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_LEFT, 20, -60);

    // CAN status label
    can_status_label = lv_label_create(main_screen);
    lv_obj_set_style_text_color(can_status_label, lv_color_hex(0xFF8000), 0);
    lv_obj_set_style_text_font(can_status_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(can_status_label, "CAN: Disconnected");
    lv_obj_align(can_status_label, LV_ALIGN_BOTTOM_LEFT, 20, -20);
}

// ============================================================================
// LDR FUNCTIONS (from proven ldr_auto_dim_test)
// ============================================================================

// Function to apply gamma correction for human perception
uint8_t applyGammaCorrection(float normalizedBrightness) {
    // Clamp input to 0.0-1.0 range
    normalizedBrightness = constrain(normalizedBrightness, 0.0f, 1.0f);
    
    // Apply gamma correction: duty = maxDuty * pow(brightness, gamma)
    float gammaCorrected = powf(normalizedBrightness, LDR_GAMMA);
    
    // Convert back to 0-255 range
    int result = (int)(gammaCorrected * (LDR_BRIGHTNESS_MAX - LDR_BRIGHTNESS_MIN) + LDR_BRIGHTNESS_MIN);
    
    // Ensure we stay within bounds
    return constrain(result, LDR_BRIGHTNESS_MIN, LDR_BRIGHTNESS_MAX);
}

// Function to update LDR averaging
void updateLdrAverage(int newReading) {
    // Remove the oldest reading from the sum
    if (ldr_data.array_filled) {
        ldr_data.sum -= ldr_data.readings[ldr_data.index];
    }
    
    // Add the new reading
    ldr_data.readings[ldr_data.index] = newReading;
    ldr_data.sum += newReading;
    
    // Move to next index
    ldr_data.index = (ldr_data.index + 1) % LDR_AVERAGE_SAMPLES;
    
    // Check if array is filled
    if (ldr_data.index == 0) {
        ldr_data.array_filled = true;
    }
    
    // Calculate average
    int sampleCount = ldr_data.array_filled ? LDR_AVERAGE_SAMPLES : ldr_data.index;
    ldr_data.averaged_value = ldr_data.sum / sampleCount;
}

uint8_t calculateBrightness(int ldrValue) {
    uint8_t targetBrightness;
    
    if (ldrValue <= LDR_DARK_THRESHOLD) {
        // Very dark environment - minimum brightness (easier on eyes)
        targetBrightness = LDR_BRIGHTNESS_MIN;
    } else if (ldrValue >= LDR_BRIGHT_THRESHOLD) {
        // Very bright environment - maximum brightness (visible in bright light)
        targetBrightness = LDR_BRIGHTNESS_MAX;
    } else {
        // Linear mapping between thresholds, then apply gamma correction
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
    // Read raw LDR value
    int ldrValue = analogRead(LDR_ANALOG_PIN);
    
    // Update the averaging
    updateLdrAverage(ldrValue);
    
    // Calculate target brightness using averaged value
    ldr_data.target_brightness = calculateBrightness(ldr_data.averaged_value);
    
    // Smooth brightness transition to reduce flicker
    if (ldr_data.brightness != ldr_data.target_brightness) {
        int brightnessDiff = ldr_data.target_brightness - ldr_data.brightness;
        ldr_data.brightness += brightnessDiff / LDR_SMOOTHING_FACTOR;
        
        // Ensure we don't overshoot
        if (abs(brightnessDiff) < LDR_SMOOTHING_FACTOR) {
            ldr_data.brightness = ldr_data.target_brightness;
        }
        
        // Constrain to valid range
        ldr_data.brightness = constrain(ldr_data.brightness, LDR_BRIGHTNESS_MIN, LDR_BRIGHTNESS_MAX);
        
        // Apply the smoothed brightness
        updateDisplayBrightness(ldr_data.brightness);
    }
    
    // Convert to percentage for display
    ldr_data.light_level = (ldrValue / 4095.0) * 100.0;
    ldr_data.valid = true;
    ldr_data.last_update = millis();
    
    // Print debug information (less frequent to avoid spam)
    static unsigned long last_debug = 0;
    if (millis() - last_debug >= 10000) { // Every 10 seconds
        Serial.printf("LDR: %d (avg: %d) | Brightness: %d (target: %d)\n", 
                     ldrValue, ldr_data.averaged_value, ldr_data.brightness, ldr_data.target_brightness);
        last_debug = millis();
    }
}

// ============================================================================
// CAN MESSAGE HANDLING
// ============================================================================

void handleCANMessages() {
    uint32_t id;
    uint8_t data[8];
    uint8_t length;

    while (can_manager->hasMessage()) {
        if (can_manager->receiveMessage(id, data, length)) {
            switch (id) {
                case CAN_MSG_SHT31_TEMP_HUMIDITY: {
                    SHT31Data* sht_data = (SHT31Data*)data;
                    if (validateChecksum(data, sizeof(SHT31Data))) {
                        sht31_data.temperature = rawToTemperature(sht_data->temperature_raw);
                        sht31_data.humidity = rawToHumidity(sht_data->humidity_raw);
                        sht31_data.temp_valid = sht_data->status_flags & 0x01;
                        sht31_data.humidity_valid = sht_data->status_flags & 0x02;
                        sht31_data.last_update = millis();
                    }
                    break;
                }
                
                case CAN_MSG_TMP36_TEMPERATURE: {
                    TMP36Data* tmp_data = (TMP36Data*)data;
                    tmp36_data.temperature = rawToTemperature(tmp_data->temperature_raw);
                    tmp36_data.temp_valid = tmp_data->status_flags & 0x01;
                    tmp36_data.last_update = millis();
                    break;
                }
                
                case CAN_MSG_HEARTBEAT: {
                    Heartbeat* hb = (Heartbeat*)data;
                    Serial.printf("Heartbeat from Node %d (Type: %d)\n", hb->device_id, hb->device_type);
                    break;
                }
                
                case CAN_MSG_ERROR_REPORT: {
                    Serial.printf("Error report: Node %d, Code: 0x%02X\n", data[1], data[2]);
                    break;
                }
            }
        }
    }
}

// ============================================================================
// UI UPDATE
// ============================================================================

void updateUI() {
    // Update temperature display
    if (sht31_data.temp_valid) {
        char temp_str[32];
        snprintf(temp_str, sizeof(temp_str), "Temperature: %.1f°C", sht31_data.temperature);
        lv_label_set_text(temp_label, temp_str);
    } else if (tmp36_data.temp_valid) {
        char temp_str[32];
        snprintf(temp_str, sizeof(temp_str), "Temperature: %.1f°C", tmp36_data.temperature);
        lv_label_set_text(temp_label, temp_str);
    }

    // Update humidity display
    if (sht31_data.humidity_valid) {
        char humidity_str[32];
        snprintf(humidity_str, sizeof(humidity_str), "Humidity: %.1f%%", sht31_data.humidity);
        lv_label_set_text(humidity_label, humidity_str);
    }

    // Update LDR display
    if (ldr_data.valid) {
        char ldr_str[64];
        snprintf(ldr_str, sizeof(ldr_str), "Light: %.1f%% (Raw: %d, Avg: %d)", 
                 ldr_data.light_level, ldr_data.readings[(ldr_data.index - 1 + LDR_AVERAGE_SAMPLES) % LDR_AVERAGE_SAMPLES], 
                 ldr_data.averaged_value);
        lv_label_set_text(ldr_label, ldr_str);
        
        // Add brightness info to status
        char brightness_str[32];
        snprintf(brightness_str, sizeof(brightness_str), "Brightness: %d/%d", 
                 ldr_data.brightness, ldr_data.target_brightness);
        // We could add this as a separate label if needed
    } else {
        lv_label_set_text(ldr_label, "Light: --%");
    }

    // Update status
    char status_str[64];
    snprintf(status_str, sizeof(status_str), "Status: OK | Messages: %d", can_manager->getMessageCount());
    lv_label_set_text(status_label, status_str);

    // Update CAN status
    lv_label_set_text(can_status_label, "CAN: Connected");
}

// ============================================================================
// MAIN SETUP
// ============================================================================

void setup() {
    Serial.begin(9600);
    delay(1000);
    
    Serial.println("=== Smart Thunderbird Display Node ===");
    printNodeConfig();
    
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
        lv_label_set_text(status_label, "Status: CAN Error");
        return;
    }
    
    g_can_manager = can_manager;
    
    Serial.println("Display node initialized successfully");
    lv_label_set_text(status_label, "Status: Ready");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    unsigned long current_time = millis();
    
    // Handle LVGL tasks
    lv_timer_handler();
    
    // Process CAN messages
    if (current_time - last_can_check >= 100) { // Check every 100ms
        handleCANMessages();
        last_can_check = current_time;
    }
    
    // Read LDR data (every 500ms for auto-dimming)
    if (current_time - last_ldr_read >= LDR_READ_INTERVAL) {
        readLDRData();
        last_ldr_read = current_time;
    }
    
    // Update UI
    if (current_time - last_sensor_update >= 1000) { // Update every 1 second
        updateUI();
        last_sensor_update = current_time;
    }
    
    // Send heartbeat
    if (current_time - last_heartbeat >= HEARTBEAT_INTERVAL) {
        can_manager->sendHeartbeat();
        last_heartbeat = current_time;
    }
    
    // Process CAN message queue
    can_manager->processQueue();
    
    delay(10);
}
