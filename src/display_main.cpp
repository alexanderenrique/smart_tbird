#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <driver/ledc.h>
#include <math.h>
#include <driver/gpio.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>


// TFT Display
TFT_eSPI tft = TFT_eSPI();

// LVGL Display Buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 4];  // Reduced buffer size for stability

// LVGL Display and Input Device
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// UI Elements
lv_obj_t *voltage_label;
lv_obj_t *afr_label;
lv_obj_t *ldr_label;
lv_obj_t *brightness_label;

// ADC Calibration
esp_adc_cal_characteristics_t adc1_chars;


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

void updateRealVoltageData() {
    // Safety check
    if (!voltage_label) return;
    
    // Read voltage from pin 35 (GPIO 35) with voltage divider (0-16V input)
    // Take multiple rapid samples and average to help with high-impedance input
    long sum = 0;
    const int num_samples = 8;  // Take 8 samples rapidly
    for (int i = 0; i < num_samples; i++) {
        sum += analogRead(35);
        delayMicroseconds(100);  // Small delay between samples
    }
    int voltage_raw = sum / num_samples;
    
    // Voltage smoothing with 1-second rolling average (10 samples at ~100ms intervals)
    static int voltage_readings[10] = {0}; // Store 10 readings (1 second at ~100ms intervals)
    static int reading_index = 0;
    static bool buffer_filled = false;
    static unsigned long last_reading_time = 0;
    
    // Only take new readings every ~100ms for smoothing
    if (millis() - last_reading_time >= 100) {
        voltage_readings[reading_index] = voltage_raw;
        reading_index = (reading_index + 1) % 10;
        if (reading_index == 0) buffer_filled = true;
        last_reading_time = millis();
    }
    
    // Calculate smoothed voltage ADC value
    int voltage_smoothed_adc;
    if (buffer_filled) {
        // Use all 10 readings for average
        long sum = 0;
        for (int i = 0; i < 10; i++) {
            sum += voltage_readings[i];
        }
        voltage_smoothed_adc = sum / 10;
    } else {
        // Use available readings
        long sum = 0;
        for (int i = 0; i < reading_index; i++) {
            sum += voltage_readings[i];
        }
        voltage_smoothed_adc = (reading_index > 0) ? (sum / reading_index) : voltage_raw;
    }
    
    // Convert ADC reading to voltage using calibrated ADC
    // ESP32 ADC with 12-bit resolution and 11dB attenuation:
    //   - Uses esp_adc_cal for accurate voltage reading
    // Voltage divider: Calibrated ratio 5.56293:1 (measured: 1.597V pin = 8.88V input)
    //   Output impedance: ~180Ω (excellent for ESP32)
    //   Range: 0-16V input → 0-2.88V on pin (safe)
    // Formula: 
    //   1. Convert ADC to calibrated pin voltage using esp_adc_cal
    //   2. Convert pin voltage to input voltage: V_input = V_pin * 5.56293
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(voltage_smoothed_adc, &adc1_chars);
    float pin_voltage = voltage_mv / 1000.0f;  // Convert mV to V
    float voltage = pin_voltage * 5.5f;
    
    // Update the display with smoothed voltage reading
    char voltage_text[20];
    sprintf(voltage_text, "Voltage: %.2f V", voltage);
    lv_label_set_text(voltage_label, voltage_text);
    
    // Force refresh of this label
    lv_obj_invalidate(voltage_label);
    
    // Serial output: ADC value and calculated voltage every second
    static unsigned long last_print = 0;
    if (millis() - last_print > 1000) {
        // Diagnostic: Calculate expected ADC for measured pin voltage
        // If you measure 0.78V on pin, expected ADC = (0.78 / 3.3) * 4095 = ~968
        Serial.printf("Pin 35 - ADC: %d, Pin V: %.3fV, Input V: %.2fV (expected ADC ~968 for 0.78V pin)\n", 
                     voltage_smoothed_adc, pin_voltage, voltage);
        last_print = millis();
    }
}

void updateRealOxygenSensorData() {
    // Safety check
    if (!afr_label) return;
    
    // O2 sensor warmup delay - 20 seconds
    static unsigned long o2_start_time = millis();
    const unsigned long O2_WARMUP_TIME = 20000; // 20 seconds in milliseconds
    
    unsigned long elapsed_time = millis() - o2_start_time;
    
    // During warmup period, display warming message
    if (elapsed_time < O2_WARMUP_TIME) {
        lv_label_set_text(afr_label, "O2 sensor warming");
        lv_obj_invalidate(afr_label);
        return; // Don't read sensor during warmup
    }
    
    // Read oxygen sensor from pin 34 (GPIO 34 = ADC1_CHANNEL_6) - 0-3.3V input
    int o2_raw = analogRead(34);
    
    // Oxygen sensor smoothing with 0.5-second rolling average (5 samples at ~100ms intervals)
    static int o2_readings[5] = {0}; // Store 5 readings (0.5 seconds at ~100ms intervals)
    static int reading_index = 0;
    static bool buffer_filled = false;
    static unsigned long last_reading_time = 0;
    
    // Only take new readings every ~100ms for smoothing
    if (millis() - last_reading_time >= 100) {
        o2_readings[reading_index] = o2_raw;
        reading_index = (reading_index + 1) % 5;
        if (reading_index == 0) buffer_filled = true;
        last_reading_time = millis();
    }
    
    // Calculate smoothed O2 ADC value
    int o2_smoothed_adc;
    if (buffer_filled) {
        // Use all 5 readings for average
        long sum = 0;
        for (int i = 0; i < 5; i++) {
            sum += o2_readings[i];
        }
        o2_smoothed_adc = sum / 5;
    } else {
        // Use available readings
        long sum = 0;
        for (int i = 0; i < reading_index; i++) {
            sum += o2_readings[i];
        }
        o2_smoothed_adc = (reading_index > 0) ? (sum / reading_index) : o2_raw;
    }
    
    // Convert ADC reading to voltage using calibrated ADC
    // ESP32 ADC with 12-bit resolution and 11dB attenuation
    // Uses esp_adc_cal for accurate voltage reading (same calibration as voltage divider)
    uint32_t o2_voltage_mv = esp_adc_cal_raw_to_voltage(o2_smoothed_adc, &adc1_chars);
    float voltage = o2_voltage_mv / 1000.0f;  // Convert mV to V
    
    // Calculate AFR: AFR = 2 * voltage + 10
    // Calibrated: 1.66V pin = 13.3 AFR
    float afr = 2.0f * voltage + 9.9f;
    
    // Update the display with AFR reading
    char afr_text[30];
    sprintf(afr_text, "AFR: %.2f", afr);
    lv_label_set_text(afr_label, afr_text);
    
    // Force refresh of this label
    lv_obj_invalidate(afr_label);
    
}

void updateBrightness(int ldr_value) {
    // Safety check
    if (!brightness_label) return;
    
    // Advanced brightness calculation with gamma correction
    // LDR 200-2000 → PWM 51-255 (20%-100%)
    // LDR <= 200 → 20% PWM, LDR >= 2000 → 100% PWM
    
    // Clamp LDR value to 200-2000 range
    if (ldr_value < 200) ldr_value = 200;
    if (ldr_value > 2000) ldr_value = 2000;
    
    // Normalize LDR to 0.0-1.0 range (200-2000 → 0.0-1.0)
    float normalized = ((float)ldr_value - 200.0f) / (2000.0f - 200.0f);
    
    // Apply gamma correction (2.2) for better human perception
    // This makes the brightness curve more perceptually linear
    float gamma_corrected = pow(normalized, 1.0f / 2.2f);
    
    // Convert to PWM value (51-255, where 51 = 20% minimum)
    const int min_brightness = 51; // 20% of 255
    const int max_brightness = 255; // 100%
    int target_brightness = min_brightness + (int)(gamma_corrected * (max_brightness - min_brightness));
    
    // Exponential smoothing for brightness transitions (smoothing factor = 8)
    // This prevents rapid flickering and makes transitions smooth
    static int current_brightness = 128; // Start at middle brightness (will adjust to min 10%)
    const float smoothing_factor = 8.0f; // Higher = slower changes
    
    // Exponential moving average: new = old + (target - old) / smoothing_factor
    current_brightness = current_brightness + (target_brightness - current_brightness) / smoothing_factor;
    
    // Ensure we're within valid range (20%-100%)
    if (current_brightness < min_brightness) current_brightness = min_brightness;
    if (current_brightness > 255) current_brightness = 255;
    
    // Set PWM brightness on pin 25 using LEDC
    ledcWrite(0, current_brightness);
    
    // Update brightness display
    char brightness_text[30];
    sprintf(brightness_text, "Brightness: %d%%", (current_brightness * 100) / 255);
    lv_label_set_text(brightness_label, brightness_text);
    
    // Force refresh of this label
    lv_obj_invalidate(brightness_label);
    
    // Note: Serial output moved to updateRealLDRData() to combine with LDR values
}

void updateRealLDRData() {
    // Safety check
    if (!ldr_label) return;
    
    // Read actual LDR value from pin 4
    int ldr_raw = analogRead(4);
    
    // Advanced LDR smoothing with 10-second rolling average (100 samples at ~100ms intervals)
    static int ldr_readings[100] = {0}; // Store 100 readings (10 seconds at ~100ms intervals)
    static int reading_index = 0;
    static bool buffer_filled = false;
    static unsigned long last_reading_time = 0;
    
    // Only take new readings every ~100ms for smoothing
    if (millis() - last_reading_time >= 100) {
        ldr_readings[reading_index] = ldr_raw;
        reading_index = (reading_index + 1) % 100;
        if (reading_index == 0) buffer_filled = true;
        last_reading_time = millis();
    }
    
    // Calculate smoothed LDR value using rolling average
    int ldr_smoothed;
    if (buffer_filled) {
        // Use all 100 readings for average
        long sum = 0;
        for (int i = 0; i < 100; i++) {
            sum += ldr_readings[i];
        }
        ldr_smoothed = sum / 100;
    } else {
        // Use available readings
        long sum = 0;
        for (int i = 0; i < reading_index; i++) {
            sum += ldr_readings[i];
        }
        ldr_smoothed = (reading_index > 0) ? (sum / reading_index) : ldr_raw;
    }
    
    // Update the display with smoothed LDR reading
    char ldr_text[30];
    sprintf(ldr_text, "LDR: %d", ldr_raw);
    lv_label_set_text(ldr_label, ldr_text);
    
    // Force refresh of this label
    lv_obj_invalidate(ldr_label);
    
    // Update brightness based on smoothed LDR reading
    updateBrightness(ldr_smoothed);
    
    // Serial output: LDR and Brightness every second
    static unsigned long last_print = 0;
    if (millis() - last_print > 1000) { // Print every second
        // Calculate brightness percentage from smoothed LDR value
        int brightness_pct = 0;
        // Use same calculation as updateBrightness() to get current brightness
        int clamped_ldr = ldr_smoothed;
        if (clamped_ldr < 200) clamped_ldr = 200;
        if (clamped_ldr > 2000) clamped_ldr = 2000;
        
        float normalized = ((float)clamped_ldr - 200.0f) / (2000.0f - 200.0f);
        float gamma_corrected = pow(normalized, 1.0f / 2.2f);
        const int min_brightness = 51; // 20% of 255
        const int max_brightness = 255; // 100%
        int target_brightness = min_brightness + (int)(gamma_corrected * (max_brightness - min_brightness));
        brightness_pct = (target_brightness * 100) / 255;
        
        Serial.printf("LDR: %d (smoothed: %d), Brightness: %d%%\n", 
                     ldr_raw, ldr_smoothed, brightness_pct);
        last_print = millis();
    }
}


void setup() {
    Serial.begin(9600);
    
    // Initialize TFT
    tft.init();
    tft.setRotation(0); // Set rotation to 0 for portrait mode
    tft.fillScreen(TFT_BLACK);
    
    // Configure ADC for voltage reading on pin 35 (GPIO 35 = ADC1_CHANNEL_7)
    // Note: ADC1 can be used even when WiFi is active (unlike ADC2)
    // Set ADC width to 12-bit (0-4095) and attenuation to 11dB (0-3.3V range)
    analogSetWidth(12);  // 12-bit resolution (0-4095)
    analogSetAttenuation(ADC_11db);  // 11dB attenuation allows 0-3.3V input range
    
    // Configure voltage pin (GPIO 35) as analog input
    pinMode(35, INPUT);
    gpio_set_pull_mode((gpio_num_t)35, GPIO_FLOATING); // Disable pull-ups/pull-downs
    
    // Characterize ADC for accurate voltage readings
    // This uses the ESP32's factory calibration data stored in eFuse
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc1_chars);
    
    // Test ADC reading immediately and show diagnostics
    delay(100);  // Let ADC settle
    int test_adc = analogRead(35);
    uint32_t test_voltage_mv = esp_adc_cal_raw_to_voltage(test_adc, &adc1_chars);
    float test_voltage = test_voltage_mv / 1000.0f;
    Serial.printf("\n=== ADC Configuration ===\n");
    Serial.printf("Pin 35 (ADC1_CHANNEL_7): 12-bit, 11dB attenuation\n");
    Serial.printf("ADC Calibration: %s\n", 
                 (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) ? "eFuse Vref" :
                 (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) ? "eFuse Two Point" :
                 "Default");
    Serial.printf("Expected ranges:\n");
    Serial.printf("  - 0.78V pin = ADC ~968\n");
    Serial.printf("  - 1.60V pin = ADC ~1985 (calibrated)\n");
    Serial.printf("Initial test - ADC: %d, Calibrated Pin Voltage: %.3fV\n", test_adc, test_voltage);
    Serial.printf("If ADC is much lower than expected, check:\n");
    Serial.printf("  1. Voltage divider resistors are correct (1kΩ/220Ω)\n");
    Serial.printf("  2. Wiring is correct\n\n");
    
    // Configure LDR pin (GPIO 4) as analog input with no pull-ups
    // Explicitly disable pull-up and pull-down resistors
    pinMode(4, INPUT);
    gpio_set_pull_mode((gpio_num_t)4, GPIO_FLOATING); // Disable pull-ups/pull-downs
    
    // Configure backlight pin as OUTPUT LOW before delay to prevent flash on boot
    pinMode(25, OUTPUT);
    digitalWrite(25, LOW);
    
    // Initialize backlight PWM using ESP32 LEDC
    // Higher frequency (25kHz) reduces visible flicker
    ledcSetup(0, 25000, 8); // Channel 0, 25kHz frequency, 8-bit resolution
    ledcAttachPin(25, 0);   // Attach pin 25 to channel 0
    ledcWrite(0, 128);       // Start at 50% brightness (will be adjusted by LDR)
    
    // Initialize LVGL
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 4);  // Reduced buffer size
    
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
    
    // Voltage Section
    voltage_label = lv_label_create(scr);
    lv_label_set_text(voltage_label, "Voltage: Initializing...");
    lv_obj_set_style_text_font(voltage_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(voltage_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(voltage_label, LV_ALIGN_TOP_LEFT, 20, 270);
    
    // Oxygen Sensor (AFR) Section
    afr_label = lv_label_create(scr);
    lv_label_set_text(afr_label, "AFR: Initializing...");
    lv_obj_set_style_text_font(afr_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(afr_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(afr_label, LV_ALIGN_TOP_LEFT, 20, 310);
    
    // LDR Section    
    ldr_label = lv_label_create(scr);
    lv_label_set_text(ldr_label, "LDR: Initializing...");
    lv_obj_set_style_text_font(ldr_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(ldr_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(ldr_label, LV_ALIGN_TOP_LEFT, 20, 350);
    
    brightness_label = lv_label_create(scr);
    lv_label_set_text(brightness_label, "Brightness: Initializing...");
    lv_obj_set_style_text_font(brightness_label, TEXT_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(brightness_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 20, 390);
    
    // Test LDR reading immediately after setup
    int initial_ldr = analogRead(4);
    char ldr_init_text[30];
    sprintf(ldr_init_text, "LDR: %d", initial_ldr);
    lv_label_set_text(ldr_label, ldr_init_text);
    
    // Initialize brightness control
    updateBrightness(initial_ldr);
}

void loop() {
    // Update sensor data with timing control
    static unsigned long last_update = 0;
    if (millis() - last_update >= 100) { // Update every 100ms instead of every 5ms
        updateRealVoltageData();
        updateRealOxygenSensorData();
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
    
    delay(10); // Increased delay to reduce CPU load
}