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
    Serial.println("Touch function is running!");
    data->state = LV_INDEV_STATE_REL;
}


void btn_event_cb(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);

  Serial.print("Event: "); 
  Serial.println(code);  // This prints the numeric code of any LVGL event received

  if (code == LV_EVENT_CLICKED) {
      press_count++;
      if (counter_label) {
          static char buf[32];
          snprintf(buf, sizeof(buf), "Presses: %d", press_count);
          lv_label_set_text(counter_label, buf);
      }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("Serial connected!");
  tft.init();
  tft.setRotation(1);

  lv_init();

  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, sizeof(buf1) / sizeof(lv_color_t));

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 480;
  disp_drv.ver_res = 320;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // UI ELEMENTS — button + label
  lv_obj_t *btn = lv_btn_create(lv_scr_act());
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, "Test");

  counter_label = lv_label_create(lv_scr_act());
  lv_obj_align(counter_label, LV_ALIGN_CENTER, 0, 60);
  lv_label_set_text(counter_label, "Presses: 0");

  // 🧩 INPUT DRIVER — register LAST
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  Serial.println("Input device registered");
}

void loop() {
  lv_timer_handler();
  delay(5);
} 