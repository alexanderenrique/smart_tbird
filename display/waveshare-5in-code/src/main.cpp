/*
 * Waveshare ESP32-S3-Touch-LCD-5B + PicoPixel LVGL UI
 */

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <lvgl.h>

#include "lvgl_v8_port.h"
#include "ui/ui.h"
#include "telemetry/rs485_modbus.h"
#include "telemetry/ui_telemetry.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

void setup()
{
    Serial.begin(115200);
    delay(200);

    Serial.println("Initializing board");
    Board *board = new Board();
    board->init();

#if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif

    assert(board->begin());

    Serial.println("Initializing LVGL");
    lvgl_port_init(board->getLCD(), board->getTouch());

    Serial.println("Creating UI");
    lvgl_port_lock(-1);
    ui_init();
    lvgl_port_unlock();

    initRs485();

    Serial.println("UI ready");
}

void loop()
{
    pollRs485();

    TelemetryData telemetry = {};
    const bool have_fresh = rs485TakeFreshTelemetry(&telemetry);

    /* lv_timer_handler runs in the LVGL port task; tick the PicoPixel UI here. */
    if (lvgl_port_lock(10)) {
        ui_tick();
        if (have_fresh) {
            ui_apply_telemetry(telemetry);
        }
        lvgl_port_unlock();
    }
    delay(5);
}
