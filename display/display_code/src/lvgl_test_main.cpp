/*
 * Minimal LVGL + TFT smoke test for display bring-up (no Modbus or sensors).
 * Build: pio run -e lvgl_test -t upload
 */

#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 4];
static lv_disp_drv_t disp_drv;

lv_obj_t *test_label;
int counter = 0;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void setup() {
    Serial.begin(115200);
    Serial.println("=== LVGL TFT smoke test ===");

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 4);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 480;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), LV_PART_MAIN);

    test_label = lv_label_create(scr);
    lv_label_set_text(test_label, "LVGL Test Starting...");
    lv_obj_set_style_text_color(test_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_align(test_label, LV_ALIGN_CENTER, 0, 0);

    Serial.println("Setup complete");
}

void loop() {
    static unsigned long last_update = 0;
    if (millis() - last_update >= 1000) {
        counter++;
        lv_label_set_text_fmt(test_label, "LVGL Test - Count: %d", counter);
        Serial.printf("Count: %d\n", counter);
        last_update = millis();
    }

    lv_timer_handler();
    delay(10);
}
