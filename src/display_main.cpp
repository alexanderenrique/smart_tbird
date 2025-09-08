#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

// TFT Display
TFT_eSPI tft = TFT_eSPI();

// LVGL Display Buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 4];  // Reduced buffer size for stability

// LVGL Display and Input Device
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// UI Elements
lv_obj_t *temp_label;
lv_obj_t *humidity_label;
lv_obj_t *accel_x_label;
lv_obj_t *accel_y_label;
lv_obj_t *accel_z_label;
lv_obj_t *voltage_label;
lv_obj_t *ldr_label;
lv_obj_t *brightness_label;

// Dummy data variables
float dummy_temp = 22.5;
float dummy_humidity = 45.0;
float dummy_accel_x = 0.1;
float dummy_accel_y = 0.2;
float dummy_accel_z = 9.8;
float dummy_voltage = 12.3;
float dummy_ldr = 512.0;  // LDR value (0-1023)

// Color scheme variables
const lv_color_t BACKGROUND_COLOR = lv_color_hex(0x191970);  // Midnight blue background
const lv_color_t TEXT_COLOR = lv_color_hex(0xFFFFFF);        // White text

// Font size variables
const lv_font_t* TITLE_FONT = &lv_font_montserrat_24;        // Main title font (larger)
const lv_font_t* TEXT_FONT = &lv_font_montserrat_30;         // Regular text font (larger)

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY);

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

void updateDummySHT31Data() {
    // Safety check
    if (!temp_label || !humidity_label) {
        Serial.println("SHT31: Labels not initialized!");
        return;
    }
    
    // Simulate realistic temperature and humidity variations
    dummy_temp += (random(-10, 11) / 100.0); // ±0.1°C variation
    dummy_humidity += (random(-20, 21) / 100.0); // ±0.2% variation
    
    // Keep values in realistic ranges
    if (dummy_temp < 15.0) dummy_temp = 15.0;
    if (dummy_temp > 35.0) dummy_temp = 35.0;
    if (dummy_humidity < 20.0) dummy_humidity = 20.0;
    if (dummy_humidity > 80.0) dummy_humidity = 80.0;
    
    char temp_text[50];
    char humidity_text[50];
    sprintf(temp_text, "Temp: %.1f°C", dummy_temp);
    sprintf(humidity_text, "Humidity: %.1f%%", dummy_humidity);
    lv_label_set_text(temp_label, temp_text);
    lv_label_set_text(humidity_label, humidity_text);
    
    // Force refresh of these labels
    lv_obj_invalidate(temp_label);
    lv_obj_invalidate(humidity_label);
    
    // Debug output to track updates
    static unsigned long last_sht31_debug = 0;
    if (millis() - last_sht31_debug > 2000) { // Print every 2 seconds
        Serial.printf("SHT31 Update - Temp: %.1f°C, Humidity: %.1f%%\n", dummy_temp, dummy_humidity);
        Serial.printf("Temp label text: %s\n", lv_label_get_text(temp_label));
        Serial.printf("Humidity label text: %s\n", lv_label_get_text(humidity_label));
        last_sht31_debug = millis();
    }
}

void updateDummyMPU6050Data() {
    // Safety check
    if (!accel_x_label || !accel_y_label || !accel_z_label) return;
    
    // Simulate realistic acceleration data
    dummy_accel_x += (random(-50, 51) / 1000.0); // ±0.05g variation
    dummy_accel_y += (random(-50, 51) / 1000.0);
    dummy_accel_z += (random(-50, 51) / 1000.0);
    
    // Keep Z-axis around 9.8g (gravity)
    if (dummy_accel_z < 9.0) dummy_accel_z = 9.0;
    if (dummy_accel_z > 10.5) dummy_accel_z = 10.5;
    
    char accel_x_text[20], accel_y_text[20], accel_z_text[20];
    sprintf(accel_x_text, "X: %.2f g", dummy_accel_x);
    sprintf(accel_y_text, "Y: %.2f g", dummy_accel_y);
    sprintf(accel_z_text, "Z: %.2f g", dummy_accel_z);
    lv_label_set_text(accel_x_label, accel_x_text);
    lv_label_set_text(accel_y_label, accel_y_text);
    lv_label_set_text(accel_z_label, accel_z_text);
    
    // Force refresh of these labels
    lv_obj_invalidate(accel_x_label);
    lv_obj_invalidate(accel_y_label);
    lv_obj_invalidate(accel_z_label);
}

void updateDummyVoltageData() {
    // Safety check
    if (!voltage_label) return;
    
    // Simulate realistic voltage data
    dummy_voltage += (random(-20, 21) / 100.0); // ±0.2V variation
    
    // Keep values in realistic ranges
    if (dummy_voltage < 11.0) dummy_voltage = 11.0;
    if (dummy_voltage > 14.0) dummy_voltage = 14.0;
    
    char voltage_text[20];
    sprintf(voltage_text, "Voltage: %.1f V", dummy_voltage);
    lv_label_set_text(voltage_label, voltage_text);
    
    // Force refresh of this label
    lv_obj_invalidate(voltage_label);

}

void updateBrightness(int ldr_value) {
    // Safety check
    if (!brightness_label) return;
    
    // Convert LDR reading (0-1023) to brightness (0-255)
    // INVERTED LOGIC: Lower LDR values = darker environment = dimmer display
    // Higher LDR values = brighter environment = brighter display
    
    int brightness;
    if (ldr_value < 100) {
        // Very dark - minimum brightness
        brightness = 30;
    } else if (ldr_value > 800) {
        // Very bright - maximum brightness
        brightness = 255;
    } else {
        // Linear mapping from 100-800 LDR to 30-255 brightness
        // Lower LDR = dimmer display
        brightness = map(ldr_value, 100, 800, 30, 255);
    }
    
    // Set PWM brightness on pin 25
    analogWrite(25, brightness);
    
    // Update brightness display
    char brightness_text[30];
    sprintf(brightness_text, "Brightness: %d%%", (brightness * 100) / 255);
    lv_label_set_text(brightness_label, brightness_text);
    
    // Force refresh of this label
    lv_obj_invalidate(brightness_label);
    
    // Debug output
    static unsigned long last_brightness_print = 0;
    if (millis() - last_brightness_print > 2000) { // Print every 2 seconds
        Serial.printf("LDR: %d -> Brightness: %d (%d%%)\n", ldr_value, brightness, (brightness * 100) / 255);
        last_brightness_print = millis();
    }
}

void updateRealLDRData() {
    // Safety check
    if (!ldr_label) {
        Serial.println("LDR: Label not initialized!");
        return;
    }
    
    // Read actual LDR value from pin 4
    int ldr_raw = analogRead(4);
    
    // LDR smoothing with 3-second rolling average
    static int ldr_readings[30] = {0}; // Store 30 readings (3 seconds at ~100ms intervals)
    static int reading_index = 0;
    static bool buffer_filled = false;
    static unsigned long last_reading_time = 0;
    
    // Only take new readings every ~100ms for smoothing
    if (millis() - last_reading_time >= 100) {
        ldr_readings[reading_index] = ldr_raw;
        reading_index = (reading_index + 1) % 30;
        if (reading_index == 0) buffer_filled = true;
        last_reading_time = millis();
    }
    
    // Calculate smoothed LDR value
    int ldr_smoothed;
    if (buffer_filled) {
        // Use all 30 readings for average
        long sum = 0;
        for (int i = 0; i < 30; i++) {
            sum += ldr_readings[i];
        }
        ldr_smoothed = sum / 30;
    } else {
        // Use available readings
        long sum = 0;
        for (int i = 0; i < reading_index; i++) {
            sum += ldr_readings[i];
        }
        ldr_smoothed = sum / reading_index;
    }
    
    // Update the display with smoothed LDR reading
    char ldr_text[30];
    sprintf(ldr_text, "LDR: %d", ldr_raw);
    lv_label_set_text(ldr_label, ldr_text);
    
    // Force refresh of this label
    lv_obj_invalidate(ldr_label);
    
    // Update brightness based on smoothed LDR reading
    updateBrightness(ldr_smoothed);
    
    // Enhanced debugging output
    static unsigned long last_print = 0;
    static int last_ldr_value = -1;
    
    if (millis() - last_print > 1000) { // Print every second
        Serial.printf("LDR Raw: %d, Smoothed: %d (Pin 4)\n", ldr_raw, ldr_smoothed);
        Serial.printf("Display label updated: %s\n", lv_label_get_text(ldr_label));
        Serial.printf("LVGL task handler called\n");
        
        // Check if smoothed value actually changed
        if (ldr_smoothed != last_ldr_value) {
            Serial.printf("LDR smoothed value changed from %d to %d\n", last_ldr_value, ldr_smoothed);
            last_ldr_value = ldr_smoothed;
        }
        
        last_print = millis();
    }
}


void setup() {
    Serial.begin(9600);
    Serial.println("=== SMART THUNDERBIRD DISPLAY NODE - DEMO MODE ===");
    
    // Initialize random seed for dummy data
    randomSeed(analogRead(0));
    Serial.println("Random seed initialized");
    
    // Initialize TFT
    Serial.println("Initializing TFT...");
    tft.init();
    tft.setRotation(0); // Set rotation to 0 for portrait mode
    tft.fillScreen(TFT_BLACK);
    Serial.println("TFT initialized");
    
    // Initialize backlight PWM
    pinMode(25, OUTPUT);
    analogWrite(25, 128); // Start at 50% brightness
    Serial.println("Backlight initialized");
    
    // Initialize LVGL
    Serial.println("Initializing LVGL...");
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 4);  // Reduced buffer size
    Serial.println("LVGL buffer initialized");
    
    // Initialize display driver
    Serial.println("Initializing display driver...");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;  // Portrait mode: width = 320
    disp_drv.ver_res = 480;  // Portrait mode: height = 480
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    Serial.println("Display driver registered");
    
    // Initialize input device driver
    Serial.println("Initializing input device...");
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    Serial.println("Input device registered");
    
    // Create UI elements
    Serial.println("Creating UI elements...");
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, BACKGROUND_COLOR, LV_PART_MAIN);
    Serial.println("Screen background set");
    
    // Title
    Serial.println("Creating title label...");
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Alex's Thunderbird");
    lv_obj_set_style_text_font(title, TITLE_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    Serial.println("Title label created");
    
    // SHT31 Section    
    Serial.println("Creating SHT31 labels...");
    temp_label = lv_label_create(scr);
    char temp_init_text[50];
    sprintf(temp_init_text, "Interior Temp: %.1f°C", dummy_temp);
    lv_label_set_text(temp_label, temp_init_text);
    lv_obj_set_style_text_font(temp_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(temp_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 20, 70);
    Serial.printf("Temp label created: %p, text: %s\n", temp_label, lv_label_get_text(temp_label));
    
    humidity_label = lv_label_create(scr);
    char humidity_init_text[50];
    sprintf(humidity_init_text, "Humidity: %.1f%%", dummy_humidity);
    lv_label_set_text(humidity_label, humidity_init_text);
    lv_obj_set_style_text_font(humidity_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(humidity_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(humidity_label, LV_ALIGN_TOP_LEFT, 20, 110);
    Serial.printf("Humidity label created: %p, text: %s\n", humidity_label, lv_label_get_text(humidity_label));
    Serial.println("SHT31 labels created");
    
    // MPU6050 Section    
    Serial.println("Creating MPU6050 labels...");
    accel_x_label = lv_label_create(scr);
    char accel_x_init_text[20];
    sprintf(accel_x_init_text, "X: %.2f g", dummy_accel_x);
    lv_label_set_text(accel_x_label, accel_x_init_text);
    lv_obj_set_style_text_font(accel_x_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(accel_x_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(accel_x_label, LV_ALIGN_TOP_LEFT, 20, 150);
    
    accel_y_label = lv_label_create(scr);
    char accel_y_init_text[20];
    sprintf(accel_y_init_text, "Y: %.2f g", dummy_accel_y);
    lv_label_set_text(accel_y_label, accel_y_init_text);
    lv_obj_set_style_text_font(accel_y_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(accel_y_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(accel_y_label, LV_ALIGN_TOP_LEFT, 20, 190);
    
    accel_z_label = lv_label_create(scr);
    char accel_z_init_text[20];
    sprintf(accel_z_init_text, "Z: %.2f g", dummy_accel_z);
    lv_label_set_text(accel_z_label, accel_z_init_text);
    lv_obj_set_style_text_font(accel_z_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(accel_z_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(accel_z_label, LV_ALIGN_TOP_LEFT, 20, 230);
    Serial.println("MPU6050 labels created");
    
    // Voltage Section
    Serial.println("Creating voltage labels...");
    voltage_label = lv_label_create(scr);
    char voltage_init_text[20];
    sprintf(voltage_init_text, "Voltage: %.1f V", dummy_voltage);
    lv_label_set_text(voltage_label, voltage_init_text);
    lv_obj_set_style_text_font(voltage_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(voltage_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(voltage_label, LV_ALIGN_TOP_LEFT, 20, 270);
    
    Serial.println("Voltage labels created");
    
    // LDR Section    
    Serial.println("Creating LDR labels...");
    ldr_label = lv_label_create(scr);
    lv_label_set_text(ldr_label, "LDR: Initializing...");
    lv_obj_set_style_text_font(ldr_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(ldr_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(ldr_label, LV_ALIGN_TOP_LEFT, 20, 310);
    Serial.printf("LDR label created: %p, text: %s\n", ldr_label, lv_label_get_text(ldr_label));
    
    brightness_label = lv_label_create(scr);
    lv_label_set_text(brightness_label, "Brightness: Initializing...");
    lv_obj_set_style_text_font(brightness_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(brightness_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 20, 350);
    Serial.printf("Brightness label created: %p, text: %s\n", brightness_label, lv_label_get_text(brightness_label));
    Serial.println("LDR labels created");
    
    // Test LDR reading immediately after setup
    int initial_ldr = analogRead(4);
    char ldr_init_text[30];
    sprintf(ldr_init_text, "LDR: %d", initial_ldr);
    lv_label_set_text(ldr_label, ldr_init_text);
    Serial.printf("Initial LDR reading: %d\n", initial_ldr);
    
    // Initialize brightness control
    updateBrightness(initial_ldr);
    
    // Status Section - Removed CAN and System status for cleaner display
    Serial.println("Status section skipped - keeping display clean");
    
    Serial.println("=== SETUP COMPLETE - RUNNING IN DEMO MODE ===");
}

void loop() {
    // Update dummy sensor data with timing control
    static unsigned long last_update = 0;
    if (millis() - last_update >= 100) { // Update every 100ms instead of every 5ms
        Serial.println("Updating sensor data...");
        updateDummySHT31Data();
        updateDummyMPU6050Data();
        updateDummyVoltageData();
        updateRealLDRData();
        last_update = millis();
    }
    
    // Handle LVGL tasks
    lv_timer_handler();
    
    // Force a full screen refresh every 500ms to ensure updates are visible
    static unsigned long last_full_refresh = 0;
    if (millis() - last_full_refresh >= 500) {
        lv_refr_now(NULL);
        last_full_refresh = millis();
    }
    
    // Debug: Print loop iteration count occasionally
    static unsigned long loop_count = 0;
    static unsigned long last_loop_debug = 0;
    loop_count++;
    
    if (millis() - last_loop_debug > 5000) { // Every 5 seconds
        Serial.printf("Loop running: %lu iterations\n", loop_count);
        last_loop_debug = millis();
    }
    
    delay(10); // Increased delay to reduce CPU load
}