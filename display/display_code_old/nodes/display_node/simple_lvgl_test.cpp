/*
 * Simple LVGL Test - Minimal Test Case
 * ====================================
 * 
 * This is the most basic LVGL test possible to debug display issues.
 * It creates a simple label and updates it every second.
 */

#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

// TFT Display
TFT_eSPI tft = TFT_eSPI();

// LVGL Display Buffer - minimal size
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 4];  // Small buffer for testing

// LVGL Display Driver
static lv_disp_drv_t disp_drv;

// Test label
lv_obj_t *test_label;

// Counter for updates
int counter = 0;

// Display flush function
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void setup() {
    Serial.begin(115200);
    Serial.println("=== SIMPLE LVGL TEST STARTING ===");
    
    // Initialize TFT
    Serial.println("Initializing TFT...");
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    Serial.println("TFT initialized");
    
    // Initialize LVGL
    Serial.println("Initializing LVGL...");
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 4);
    Serial.println("LVGL buffer initialized");
    
    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 480;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    Serial.println("LVGL display driver registered");
    
    // Create simple test label
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);
    
    test_label = lv_label_create(scr);
    lv_label_set_text(test_label, "LVGL Test Starting...");
    lv_obj_set_style_text_color(test_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(test_label, LV_ALIGN_CENTER, 0, 0);
    
    Serial.println("Test label created");
    Serial.println("=== SETUP COMPLETE ===");
}

void loop() {
    // Update counter every second
    static unsigned long last_update = 0;
    if (millis() - last_update >= 1000) {
        counter++;
        lv_label_set_text_fmt(test_label, "LVGL Test - Count: %d", counter);
        Serial.printf("Updated label - Count: %d\n", counter);
        last_update = millis();
    }
    
    // Handle LVGL tasks
    lv_timer_handler();
    
    // Small delay
    delay(10);
}
