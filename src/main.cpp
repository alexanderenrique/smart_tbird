#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

// TFT Display
TFT_eSPI tft = TFT_eSPI();

// LVGL Display Buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 10];

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
lv_obj_t *current_label;
lv_obj_t *power_label;
lv_obj_t *ldr_label;
lv_obj_t *brightness_label;
lv_obj_t *can_status_label;
lv_obj_t *system_status_label;

// Dummy data variables
float dummy_temp = 22.5;
float dummy_humidity = 45.0;
float dummy_accel_x = 0.1;
float dummy_accel_y = 0.2;
float dummy_accel_z = 9.8;
float dummy_voltage = 12.3;
float dummy_current = 2.1;
float dummy_power = 25.8;
float dummy_ldr = 512.0;  // LDR value (0-1023)

// Color scheme variables
const lv_color_t BACKGROUND_COLOR = lv_color_hex(0x191970);  // Midnight blue background
const lv_color_t TEXT_COLOR = lv_color_hex(0xFFFFFF);        // White text
const lv_color_t HEADER_COLOR = lv_color_hex(0x00BFFF);      // Light blue headers
const lv_color_t WARNING_COLOR = lv_color_hex(0xFFFF00);     // Yellow warnings
const lv_color_t SUCCESS_COLOR = lv_color_hex(0x00FF00);     // Green success
const lv_color_t ERROR_COLOR = lv_color_hex(0xFF0000);       // Red errors

// Font size variables
const lv_font_t* TITLE_FONT = &lv_font_montserrat_24;        // Main title font
const lv_font_t* TEXT_FONT = &lv_font_montserrat_18;         // Regular text font

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
    // Simulate realistic temperature and humidity variations
    dummy_temp += (random(-10, 11) / 100.0); // ±0.1°C variation
    dummy_humidity += (random(-20, 21) / 100.0); // ±0.2% variation
    
    // Keep values in realistic ranges
    if (dummy_temp < 15.0) dummy_temp = 15.0;
    if (dummy_temp > 35.0) dummy_temp = 35.0;
    if (dummy_humidity < 20.0) dummy_humidity = 20.0;
    if (dummy_humidity > 80.0) dummy_humidity = 80.0;
    
    lv_label_set_text_fmt(temp_label, "Interior Temp: %.1f°C", dummy_temp);
    lv_label_set_text_fmt(humidity_label, "Interior Humidity: %.1f%%", dummy_humidity);
    
    // Force display refresh by invalidating the label areas
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
    // Simulate realistic acceleration data
    dummy_accel_x += (random(-50, 51) / 1000.0); // ±0.05g variation
    dummy_accel_y += (random(-50, 51) / 1000.0);
    dummy_accel_z += (random(-50, 51) / 1000.0);
    
    // Keep Z-axis around 9.8g (gravity)
    if (dummy_accel_z < 9.0) dummy_accel_z = 9.0;
    if (dummy_accel_z > 10.5) dummy_accel_z = 10.5;
    
    lv_label_set_text_fmt(accel_x_label, "X: %.2f g", dummy_accel_x);
    lv_label_set_text_fmt(accel_y_label, "Y: %.2f g", dummy_accel_y);
    lv_label_set_text_fmt(accel_z_label, "Z: %.2f g", dummy_accel_z);
    
    // Force display refresh
    lv_obj_invalidate(accel_x_label);
    lv_obj_invalidate(accel_y_label);
    lv_obj_invalidate(accel_z_label);
}

void updateDummyINA219Data() {
    // Simulate realistic power monitoring data
    dummy_voltage += (random(-20, 21) / 100.0); // ±0.2V variation
    dummy_current += (random(-10, 11) / 100.0); // ±0.1A variation
    
    // Keep values in realistic ranges
    if (dummy_voltage < 11.0) dummy_voltage = 11.0;
    if (dummy_voltage > 14.0) dummy_voltage = 14.0;
    if (dummy_current < 0.5) dummy_current = 0.5;
    if (dummy_current > 5.0) dummy_current = 5.0;
    
    dummy_power = dummy_voltage * dummy_current;
    
    lv_label_set_text_fmt(voltage_label, "Voltage: %.1f V", dummy_voltage);
    lv_label_set_text_fmt(current_label, "Current: %.1f A", dummy_current);
    lv_label_set_text_fmt(power_label, "Power: %.1f W", dummy_power);
    
    // Force display refresh
    lv_obj_invalidate(voltage_label);

}

void updateBrightness(int ldr_value) {
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
    lv_label_set_text_fmt(brightness_label, "Brightness: %d%%", (brightness * 100) / 255);
    lv_obj_invalidate(brightness_label);
    
    // Debug output
    static unsigned long last_brightness_print = 0;
    if (millis() - last_brightness_print > 2000) { // Print every 2 seconds
        Serial.printf("LDR: %d -> Brightness: %d (%d%%)\n", ldr_value, brightness, (brightness * 100) / 255);
        last_brightness_print = millis();
    }
}

void updateRealLDRData() {
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
    lv_label_set_text_fmt(ldr_label, "LDR: %d (avg: %d)", ldr_raw, ldr_smoothed);
    
    // Force display refresh by invalidating the label area
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

void updateDummySystemStatus() {
    // Simulate system status changes
    static int status_counter = 0;
    status_counter++;
    
    if (status_counter % 100 == 0) {
        lv_label_set_text(system_status_label, "System: OK");
        lv_obj_set_style_text_color(system_status_label, SUCCESS_COLOR, LV_PART_MAIN);
    } else if (status_counter % 100 == 50) {
        lv_label_set_text(system_status_label, "System: WARNING");
        lv_obj_set_style_text_color(system_status_label, WARNING_COLOR, LV_PART_MAIN);
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println("Smart Thunderbird Display Node - DEMO MODE");
    
    // Initialize random seed for dummy data
    randomSeed(analogRead(0));
    
    // Initialize TFT
    tft.init();
    tft.setRotation(0); // Set rotation to 0 for portrait mode
    tft.fillScreen(TFT_BLACK);
    
    // Initialize backlight PWM
    pinMode(25, OUTPUT);
    analogWrite(25, 128); // Start at 50% brightness
    
    // Initialize LVGL
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 10);
    
    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;  // Portrait mode: width = 320
    disp_drv.ver_res = 480;  // Portrait mode: height = 480
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    // Initialize input device driver
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    
    // Create UI elements
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, BACKGROUND_COLOR, LV_PART_MAIN);
    
    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, "Alex's Thunderbird");
    lv_obj_set_style_text_font(title, TITLE_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // SHT31 Section    
    temp_label = lv_label_create(scr);
    lv_label_set_text_fmt(temp_label, "InteriorTemp: %.1f°C", dummy_temp);
    lv_obj_set_style_text_font(temp_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(temp_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 20, 80);
    
    humidity_label = lv_label_create(scr);
    lv_label_set_text_fmt(humidity_label, "Interior Humidity: %.1f%%", dummy_humidity);
    lv_obj_set_style_text_font(humidity_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(humidity_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(humidity_label, LV_ALIGN_TOP_LEFT, 20, 105);
    
    // MPU6050 Section    
    accel_x_label = lv_label_create(scr);
    lv_label_set_text_fmt(accel_x_label, "X: %.2f g", dummy_accel_x);
    lv_obj_set_style_text_font(accel_x_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(accel_x_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(accel_x_label, LV_ALIGN_TOP_LEFT, 20, 170);
    
    accel_y_label = lv_label_create(scr);
    lv_label_set_text_fmt(accel_y_label, "Y: %.2f g", dummy_accel_y);
    lv_obj_set_style_text_font(accel_y_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(accel_y_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(accel_y_label, LV_ALIGN_TOP_LEFT, 20, 195);
    
    accel_z_label = lv_label_create(scr);
    lv_label_set_text_fmt(accel_z_label, "Z: %.2f g", dummy_accel_z);
    lv_obj_set_style_text_font(accel_z_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(accel_z_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(accel_z_label, LV_ALIGN_TOP_LEFT, 20, 220);
    
    // INA219 Section
    voltage_label = lv_label_create(scr);
    lv_label_set_text_fmt(voltage_label, "Voltage: %.1f V", dummy_voltage);
    lv_obj_set_style_text_font(voltage_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(voltage_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(voltage_label, LV_ALIGN_TOP_LEFT, 20, 285);
    
    // LDR Section    
    ldr_label = lv_label_create(scr);
    lv_label_set_text(ldr_label, "LDR: Initializing...");
    lv_obj_set_style_text_font(ldr_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(ldr_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(ldr_label, LV_ALIGN_TOP_LEFT, 20, 385);
    
    brightness_label = lv_label_create(scr);
    lv_label_set_text(brightness_label, "Brightness: Initializing...");
    lv_obj_set_style_text_font(brightness_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(brightness_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 20, 410);
    
    // Test LDR reading immediately after setup
    int initial_ldr = analogRead(4);
    lv_label_set_text_fmt(ldr_label, "LDR: %d (Initial)", initial_ldr);
    Serial.printf("Initial LDR reading: %d\n", initial_ldr);
    
    // Initialize brightness control
    updateBrightness(initial_ldr);
    
    // Status Section
    can_status_label = lv_label_create(scr);
    lv_label_set_text(can_status_label, "CAN: DEMO");
    lv_obj_set_style_text_font(can_status_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(can_status_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(can_status_label, LV_ALIGN_TOP_RIGHT, -20, 50);
    
    system_status_label = lv_label_create(scr);
    lv_label_set_text(system_status_label, "System: OK");
    lv_obj_set_style_text_font(system_status_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(system_status_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(system_status_label, LV_ALIGN_TOP_RIGHT, -20, 80);
    
    Serial.println("RUNNING IN DEMO MODE - Displaying dummy sensor data");
}

void loop() {
    // Update dummy sensor data
    updateDummySHT31Data();
    updateDummyMPU6050Data();
    updateDummyINA219Data();
    updateRealLDRData();
    updateDummySystemStatus();
    
    // Handle LVGL tasks
    lv_timer_handler();
    
    // Force screen refresh every few iterations to ensure updates are visible
    static int refresh_counter = 0;
    refresh_counter++;
    if (refresh_counter % 10 == 0) { // Every 10 loop iterations
        lv_refr_now(NULL); // Force immediate screen refresh
    }
    
    // Debug: Print loop iteration count occasionally
    static unsigned long loop_count = 0;
    static unsigned long last_loop_debug = 0;
    loop_count++;
    
    if (millis() - last_loop_debug > 5000) { // Every 5 seconds
        Serial.printf("Loop running: %lu iterations\n", loop_count);
        last_loop_debug = millis();
    }
    
    delay(5);
}