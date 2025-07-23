#include <Arduino.h>

#define LV_CONF_INCLUDE_SIMPLE
#include <lv_demo.h>
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

void setup() {
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

  // Optional: Load a basic LVGL widget
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Hello, LVGL!");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  // lv_demo_widgets();
  char *demo_info[] = { (char*)"widgets" };
  lv_demos_create(demo_info, 1);
}

void loop() {
  lv_timer_handler();  // Run LVGL tasks
  delay(5);            // Call every ~5ms
}