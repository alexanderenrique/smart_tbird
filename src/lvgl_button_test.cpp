#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

TFT_eSPI tft = TFT_eSPI();

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[480 * 4];  // Reduced buffer size for memory safety

// Counter variable and label pointer
static int press_count = 0;
static lv_obj_t *counter_label = NULL;

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

void btn_event_cb(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);

  // Only respond to actual button clicks, not other events
  if (code == LV_EVENT_CLICKED) {
      press_count++;
      if (counter_label) {
          static char buf[32];
          snprintf(buf, sizeof(buf), "Presses: %d", press_count);
          lv_label_set_text(counter_label, buf);
          Serial.printf("✅ Button pressed! Count: %d\n", press_count);
      }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("🚀 Smart Thunderbird starting up...");

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

  // 4. Create UI elements
  lv_obj_t *btn = lv_btn_create(lv_scr_act());
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);  // Only listen for clicks
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, "Press Me!");

  counter_label = lv_label_create(lv_scr_act());
  lv_obj_align(counter_label, LV_ALIGN_CENTER, 0, 60);
  lv_label_set_text(counter_label, "Presses: 0");

  // 5. Initialize and register input driver
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_t *my_indev = lv_indev_drv_register(&indev_drv);

  // 6. Debug print to confirm input registration
  if (!my_indev) {
      Serial.println("🚨 LVGL: Failed to register input driver!");
  } else {
      Serial.println("✅ LVGL: Input driver registered successfully.");
  }

  Serial.println("🎯 Ready! Touch the button to test.");
}

void loop() {
  lv_tick_inc(5);
  lv_timer_handler();
  delay(5);
} 