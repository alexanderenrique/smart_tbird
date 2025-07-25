#include <Arduino.h>

#include "widgets/lv_demo_widgets.h"
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();  // Uses your User_Setup.h config

// Display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[480 * 10];  // Line buffer (adjust as needed)

// Flush function to transfer LVGL buffer to display
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, area->x2 - area->x1 + 1, area->y2 - area->y1 + 1);
  tft.pushColors((uint16_t *)&color_p->full, (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1), true);
  tft.endWrite();

  lv_disp_flush_ready(disp);  // Tell LVGL we're done
}

// Touch input driver for LVGL
void my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
    uint16_t touchX, touchY;
    if (tft.getTouch(&touchX, &touchY, 100)) { // 600 = touch threshold, adjust if needed
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
        Serial.print("Touch: X=");
        Serial.print(touchX);
        Serial.print(" Y=");
        Serial.println(touchY);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Booting..."); // or similar
  tft.init();
  tft.setRotation(1);
  lv_init();

  // Init buffer
  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, sizeof(buf1) / sizeof(lv_color_t));

  // Register display driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 480;
  disp_drv.ver_res = 320;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Register touch input device with LVGL
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // Optional: Load a basic LVGL widget
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Hello, LVGL!");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  lv_demo_widgets();
}

void loop() {
  uint16_t x, y;
  if (tft.getTouch(&x, &y, 100)) {
    Serial.print("Direct Touch: X=");
    Serial.print(x);
    Serial.print(" Y=");
    Serial.println(y);
  }
  lv_timer_handler();  // Run LVGL tasks
  delay(5);            // Call every ~5ms
}