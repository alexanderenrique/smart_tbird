#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <WiFi.h>
#include <WebServer.h>

TFT_eSPI tft = TFT_eSPI();

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[480 * 4];

// WiFi AP Configuration
const char* ssid = "SmartThunderbird";
const char* password = "12345678";
const int port = 8080;

// Web server for receiving sensor data
WebServer server(port);

// Display variables
static int press_count = 0;
static lv_obj_t *counter_label = NULL;
static lv_obj_t *temp_label = NULL;
static lv_obj_t *status_label = NULL;
static lv_obj_t *ip_label = NULL;

// Temperature data from sensor
static float received_temp = 0.0;
static String sensor_status = "No sensor connected";

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

// Web server handlers
void handleRoot() {
    String html = "<html><body>";
    html += "<h1>Smart Thunderbird Sensor Hub</h1>";
    html += "<p>Button Presses: " + String(press_count) + "</p>";
    html += "<p>Temperature: " + String(received_temp, 2) + "°C</p>";
    html += "<p>Status: " + sensor_status + "</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleSensorData() {
    if (server.hasArg("temp")) {
        received_temp = server.arg("temp").toFloat();
        sensor_status = "Connected";
        
        // Update display
        if (temp_label) {
            static char temp_buf[32];
            snprintf(temp_buf, sizeof(temp_buf), "%.1f°C", received_temp);
            lv_label_set_text(temp_label, temp_buf);
        }
        
        if (status_label) {
            static char status_buf[64];
            const char* temp_status = "Normal";
            if (received_temp < 0) temp_status = "Cold";
            else if (received_temp < 20) temp_status = "Cool";
            else if (received_temp < 30) temp_status = "Normal";
            else if (received_temp < 40) temp_status = "Warm";
            else temp_status = "Hot";
            
            snprintf(status_buf, sizeof(status_buf), "Status: %s", temp_status);
            lv_label_set_text(status_label, status_buf);
            
            // Change color based on temperature
            if (received_temp > 30) {
                lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF0000), 0); // Red
            } else if (received_temp < 10) {
                lv_obj_set_style_text_color(status_label, lv_color_hex(0x0000FF), 0); // Blue
            } else {
                lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0); // Green
            }
        }
        
        Serial.printf("📡 Received temperature: %.2f°C\n", received_temp);
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Missing temperature data");
    }
}

void setupWiFiAP() {
    Serial.println("📡 Setting up WiFi Access Point...");
    
    // Configure WiFi AP
    WiFi.softAP(ssid, password);
    
    // Wait for AP to start
    delay(1000);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.printf("🌐 AP IP address: %s\n", IP.toString().c_str());
    
    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/sensor", HTTP_POST, handleSensorData);
    
    server.begin();
    Serial.println("✅ Web server started on port " + String(port));
}

void createUI() {
    // Button
    lv_obj_t *btn = lv_btn_create(lv_scr_act());
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, "Press Me!");

    // Counter label
    counter_label = lv_label_create(lv_scr_act());
    lv_obj_align(counter_label, LV_ALIGN_TOP_LEFT, 20, 80);
    lv_label_set_text(counter_label, "Presses: 0");

    // IP Address label
    ip_label = lv_label_create(lv_scr_act());
    lv_obj_align(ip_label, LV_ALIGN_TOP_LEFT, 20, 120);
    lv_label_set_text(ip_label, "IP: 192.168.4.1");

    // Temperature label
    temp_label = lv_label_create(lv_scr_act());
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 20, 160);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(temp_label, "0.0°C");

    // Status label
    status_label = lv_label_create(lv_scr_act());
    lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 20, 200);
    lv_label_set_text(status_label, "Status: Waiting for sensor...");
}

void setup() {
    Serial.begin(9600);
    Serial.println("🚀 Smart Thunderbird AP Display starting up...");

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

    // 4. Create UI
    createUI();

    // 5. Setup WiFi AP
    setupWiFiAP();

    // 6. Initialize and register input driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_t *my_indev = lv_indev_drv_register(&indev_drv);

    if (!my_indev) {
        Serial.println("🚨 LVGL: Failed to register input driver!");
    } else {
        Serial.println("✅ LVGL: Input driver registered successfully.");
    }

    Serial.println("🎯 Ready! Connect sensor to WiFi: " + String(ssid));
}

void loop() {
    lv_tick_inc(5);
    lv_timer_handler();
    
    // Handle web server requests
    server.handleClient();
    
    delay(5);
} 