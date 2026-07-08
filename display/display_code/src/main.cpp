#include <Arduino.h>
#include "tft_setup.h"  // TOUCH_CS and TFT pins (also force-included via platformio.ini)
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include "pins.h"
#include "touch_debug.h"
#include "ui.h"
#if ENABLE_MODBUS_RTU
#include "ModbusClientRTU.h"
#include "RTUutils.h"
#include "modbus_config.h"
#endif

TFT_eSPI tft = TFT_eSPI();

// Set to 1 to sweep backlight 0%→100%→0% for hardware bring-up (disables LDR auto-dim).
#define BACKLIGHT_SWEEP_TEST 0

#if BACKLIGHT_SWEEP_TEST
static constexpr int BACKLIGHT_DUTY_MIN = 0;
static constexpr int BACKLIGHT_DUTY_MAX = 255;
static constexpr unsigned long BACKLIGHT_SWEEP_HALF_MS = 10000;
#endif

static int backlightDuty(int brightness_0_to_255) {
    if (Pins::BACKLIGHT_ACTIVE_HIGH) {
        return brightness_0_to_255;
    }
    return 255 - brightness_0_to_255;
}

static constexpr int LVGL_BUF_LINES = 10;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * LVGL_BUF_LINES];

static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

#if ENABLE_MODBUS_RTU
HardwareSerial RS485(2);
ModbusClientRTU MBclient(Pins::MODBUS_DE_RE);

static uint16_t modbus_regs[ModbusConfig::REG_COUNT] = {};
static bool modbus_data_valid = false;
#endif

static void bootLog(const char *msg) {
    ESP_LOGI("display", "%s", msg);
    Serial.println(msg);
    Serial.flush();
    delay(10);
}

static void logBootHealth() {
    const esp_reset_reason_t reason = esp_reset_reason();
    ESP_LOGI("display", "reset reason=%d psram=%s free_int=%u free_psram=%u",
             (int)reason,
             psramFound() ? "yes" : "no",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    Serial.printf("reset reason=%d psram=%s free_int=%u free_psram=%u\n",
                  (int)reason,
                  psramFound() ? "yes" : "no",
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    Serial.flush();
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

#if defined(TOUCH_CS) && (TOUCH_CS >= 0)
static constexpr int DISPLAY_WIDTH = 320;
static constexpr int DISPLAY_HEIGHT = 480;

// Touch input read callback for LVGL (matches NEMO display-firmware)
void touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    uint16_t x = 0;
    uint16_t y = 0;
    bool touched = tft.getTouch(&x, &y, TOUCH_PRESSURE_THRESHOLD);

    static bool was_pressed = false;
    static int last_lvgl_x = 0;
    static int last_lvgl_y = 0;

    if (touched) {
        if (x >= DISPLAY_WIDTH) x = DISPLAY_WIDTH - 1;
        if (y >= DISPLAY_HEIGHT) y = DISPLAY_HEIGHT - 1;
        last_lvgl_x = x;
        last_lvgl_y = (DISPLAY_HEIGHT - 1) - y;
        data->point.x = last_lvgl_x;
        data->point.y = last_lvgl_y;
        data->state = LV_INDEV_STATE_PRESSED;
        if (!was_pressed) {
#if TOUCH_DEBUG
            TouchDebug::logPressDetail(tft, x, y, last_lvgl_x, last_lvgl_y);
#else
            Serial.print("Touch: pressed at (");
            Serial.print(x);
            Serial.print(", ");
            Serial.print(y);
            Serial.println(")");
#endif
            was_pressed = true;
        }
    } else {
        data->point.x = last_lvgl_x;
        data->point.y = last_lvgl_y;
        data->state = LV_INDEV_STATE_RELEASED;
        if (was_pressed) {
            Serial.println("Touch: released");
            was_pressed = false;
        }
    }
}
#endif

#if BACKLIGHT_SWEEP_TEST
void updateBacklightSweep() {
    if (!Pins::backlightPinAssigned()) return;

    const unsigned long cycle_ms = BACKLIGHT_SWEEP_HALF_MS * 2;
    const unsigned long phase_ms = millis() % cycle_ms;
    int brightness;

    if (phase_ms < BACKLIGHT_SWEEP_HALF_MS) {
        brightness = BACKLIGHT_DUTY_MIN +
                     (int)((long)(BACKLIGHT_DUTY_MAX - BACKLIGHT_DUTY_MIN) * phase_ms / BACKLIGHT_SWEEP_HALF_MS);
    } else {
        const unsigned long down_ms = phase_ms - BACKLIGHT_SWEEP_HALF_MS;
        brightness = BACKLIGHT_DUTY_MAX -
                     (int)((long)(BACKLIGHT_DUTY_MAX - BACKLIGHT_DUTY_MIN) * down_ms / BACKLIGHT_SWEEP_HALF_MS);
    }

    ledcWrite(0, backlightDuty(brightness));

    static unsigned long last_label_update = 0;
    if (millis() - last_label_update >= 100) {
        ui_update_brightness_pct((brightness * 100) / 255);
        last_label_update = millis();
    }
}
#else
void updateBrightnessData(int ldr_value) {
    if (ldr_value < 200) ldr_value = 200;
    if (ldr_value > 2000) ldr_value = 2000;

    float normalized = ((float)ldr_value - 200.0f) / (2000.0f - 200.0f);
    float gamma_corrected = normalized * normalized;

    const int min_brightness = 51;
    const int max_brightness = 255;
    int target_brightness = min_brightness + (int)(gamma_corrected * (max_brightness - min_brightness));

    static int current_brightness = 128;
    const float smoothing_factor = 8.0f;

    current_brightness = current_brightness + (target_brightness - current_brightness) / smoothing_factor;

    if (current_brightness < min_brightness) current_brightness = min_brightness;
    if (current_brightness > max_brightness) current_brightness = max_brightness;

    if (Pins::backlightPinAssigned()) {
        ledcWrite(0, backlightDuty(current_brightness));
    }

    ui_update_brightness_pct((current_brightness * 100) / 255);
}
#endif

#if !BACKLIGHT_SWEEP_TEST
void updateLDRData() {
    if (!Pins::ldrPinAssigned()) return;

    int ldr_raw = analogRead(Pins::LDR_ADC);

    static int ldr_readings[100] = {};
    static int reading_index = 0;
    static bool buffer_filled = false;
    static unsigned long last_reading_time = 0;

    if (millis() - last_reading_time >= 100) {
        ldr_readings[reading_index] = ldr_raw;
        reading_index = (reading_index + 1) % 100;
        if (reading_index == 0) buffer_filled = true;
        last_reading_time = millis();
    }

    int ldr_smoothed;
    if (buffer_filled) {
        long sum = 0;
        for (int i = 0; i < 100; i++) {
            sum += ldr_readings[i];
        }
        ldr_smoothed = sum / 100;
    } else {
        long sum = 0;
        for (int i = 0; i < reading_index; i++) {
            sum += ldr_readings[i];
        }
        ldr_smoothed = (reading_index > 0) ? (sum / reading_index) : ldr_raw;
    }

    updateBrightnessData(ldr_smoothed);
}
#endif

#if ENABLE_MODBUS_RTU
void handleModbusData(ModbusMessage response, uint32_t token) {
    if (response.getFunctionCode() != READ_HOLD_REGISTER) return;

    const uint8_t byteCount = response[2];
    const unsigned neededBytes = ModbusConfig::REG_COUNT * 2;
    if (byteCount < neededBytes || response.size() < 3 + neededBytes) return;

    for (unsigned i = 0; i < ModbusConfig::REG_COUNT; i++) {
        const unsigned idx = 3 + (i * 2);
        modbus_regs[i] = (response[idx] << 8) | response[idx + 1];
    }
    modbus_data_valid = true;
}

void handleModbusError(Error error, uint32_t token) {
    Serial.printf("Modbus error: %02X\n", error);
    modbus_data_valid = false;
}

void updateModbusDisplay() {
    UiDashboardData data = {};
    data.valid = modbus_data_valid;

    if (modbus_data_valid) {
        data.oil_temp_f = ModbusConfig::tempFromRegister(modbus_regs[ModbusConfig::REG_OIL_TEMP]);
        data.coolant_temp_f = ModbusConfig::tempFromRegister(modbus_regs[ModbusConfig::REG_COOLANT_TEMP]);
        data.trans_temp_f = ModbusConfig::tempFromRegister(modbus_regs[ModbusConfig::REG_TRANS_TEMP]);
        data.o2_afr = ModbusConfig::afrFromRegister(modbus_regs[ModbusConfig::REG_AFR]);
        data.voltage_v = ModbusConfig::voltageFromRegister(modbus_regs[ModbusConfig::REG_VOLTAGE]);
    }

    ui_update_dashboard(&data);
}

void updateModbusData() {
    updateModbusDisplay();

    static unsigned long last_request = 0;
    static uint32_t request_token = 1;

    if (!Pins::modbusPinsAssigned()) return;

    if (millis() - last_request >= ModbusConfig::POLL_INTERVAL_MS) {
        Error err = MBclient.addRequest(
            request_token++,
            ModbusConfig::SLAVE_ID,
            READ_HOLD_REGISTER,
            ModbusConfig::REG_START,
            ModbusConfig::REG_COUNT);
        if (err != SUCCESS) {
            Serial.printf("Failed to add Modbus request: %02X\n", err);
        }
        last_request = millis();
    }
}

void initModbus() {
    if (!Pins::modbusPinsAssigned()) {
        Serial.println("WARN: Modbus pins not assigned in pins.h — skipping RS-485 init");
        return;
    }

    RTUutils::prepareHardwareSerial(RS485);
    RS485.begin(ModbusConfig::BAUD_RATE, SERIAL_8N1, Pins::MODBUS_RX, Pins::MODBUS_TX);

    MBclient.begin(RS485);
    MBclient.setTimeout(ModbusConfig::TIMEOUT_MS);
    MBclient.onDataHandler(&handleModbusData);
    MBclient.onErrorHandler(&handleModbusError);

    Serial.println("Modbus RTU client initialized");
}
#endif  // ENABLE_MODBUS_RTU

void initLdrAndBacklight() {
    if (BACKLIGHT_AUTO_DIM) {
        analogReadResolution(12);
    }

    if (Pins::ldrPinAssigned()) {
        pinMode(Pins::LDR_ADC, INPUT);
        analogSetPinAttenuation(Pins::LDR_ADC, ADC_11db);
        Serial.printf("LDR on GPIO %d\n", Pins::LDR_ADC);
    } else {
        Serial.println("LDR auto-dim disabled (no sensor or BACKLIGHT_AUTO_DIM=0)");
    }

    if (Pins::backlightPinAssigned()) {
        pinMode(Pins::BACKLIGHT_PWM, OUTPUT);
        digitalWrite(Pins::BACKLIGHT_PWM, Pins::BACKLIGHT_ACTIVE_HIGH ? LOW : HIGH);
        ledcSetup(0, Pins::BACKLIGHT_PWM_HZ, Pins::BACKLIGHT_PWM_BITS);
        ledcAttachPin(Pins::BACKLIGHT_PWM, 0);
#if BACKLIGHT_SWEEP_TEST
        ledcWrite(0, backlightDuty(BACKLIGHT_DUTY_MIN));
        Serial.printf("Backlight sweep on GPIO %d: %u Hz, 0%%-100%% over %lu s each way (active-%s)\n",
                      Pins::BACKLIGHT_PWM, Pins::BACKLIGHT_PWM_HZ, BACKLIGHT_SWEEP_HALF_MS / 1000,
                      Pins::BACKLIGHT_ACTIVE_HIGH ? "high" : "low");
#else
        const int duty = BACKLIGHT_AUTO_DIM ? 128 : Pins::BACKLIGHT_FIXED_DUTY;
        ledcWrite(0, backlightDuty(duty));
        Serial.printf("Backlight PWM on GPIO %d: %u Hz duty=%d (active-%s, auto_dim=%s)\n",
                      Pins::BACKLIGHT_PWM, Pins::BACKLIGHT_PWM_HZ, duty,
                      Pins::BACKLIGHT_ACTIVE_HIGH ? "high" : "low",
                      BACKLIGHT_AUTO_DIM ? "on" : "fixed");
#endif
    } else {
        Serial.println("WARN: BACKLIGHT_PWM not assigned in pins.h");
    }
}

void initDisplay() {
    bootLog("TFT: init...");
    tft.init();
    bootLog("TFT: setRotation");
    tft.setRotation(0);
    bootLog("TFT: fillScreen");
    tft.fillScreen(TFT_BLACK);
    bootLog("TFT: done");
}

void initLvgl() {
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * LVGL_BUF_LINES);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 320;
    disp_drv.ver_res = 480;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

#if defined(TOUCH_CS) && (TOUCH_CS >= 0)
    char touchMsg[64];
    snprintf(touchMsg, sizeof(touchMsg), "Touch: LVGL input enabled (T_CS=GPIO %d)", TOUCH_CS);
    bootLog(touchMsg);
#if TOUCH_DEBUG
    TouchDebug::logPinConfig();
    bootLog("Touch: running boot raw sample...");
    TouchDebug::logRawSample(tft, "boot");
#endif
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read;
    lv_indev_drv_register(&indev_drv);
#else
    bootLog("Touch: DISABLED — TOUCH_CS not defined at compile time");
#endif
}

void setup() {
    Serial.begin(115200);
    delay(2000);  // USB CDC on ESP32-S3 needs extra time before first log
    logBootHealth();
    bootLog("\n=== Smart Thunderbird Display (ESP32-S3) ===");

    initDisplay();
    bootLog("initLdrAndBacklight...");
    initLdrAndBacklight();
    bootLog("initLvgl...");
    initLvgl();
    bootLog("ui_init...");
    ui_init();
    bootLog("ui_init done");
    for (int i = 0; i < 5; i++) {
        lv_timer_handler();
        delay(5);
    }
#if !BACKLIGHT_SWEEP_TEST && !BACKLIGHT_AUTO_DIM
    ui_update_brightness_pct((Pins::BACKLIGHT_FIXED_DUTY * 100) / 255);
#endif
#if TOUCH_DEBUG && defined(TOUCH_CS) && (TOUCH_CS >= 0)
    bootLog("Touch: post-UI raw sample...");
    TouchDebug::logRawSample(tft, "post-ui");
#endif
#if ENABLE_MODBUS_RTU
    initModbus();
#else
    ui_set_dashboard_placeholders();
#endif

    bootLog("Setup complete");
}

void loop() {
    // LVGL tick + handler (matches NEMO: 5 ms loop for responsive touch polling)
    lv_tick_inc(5);
    lv_timer_handler();

#if BACKLIGHT_SWEEP_TEST
    updateBacklightSweep();
#endif

    static unsigned long last_heartbeat = 0;
    if (millis() - last_heartbeat >= 5000) {
        bootLog("heartbeat");
        last_heartbeat = millis();
    }

    static unsigned long last_update = 0;
    if (millis() - last_update >= 100) {
#if !BACKLIGHT_SWEEP_TEST
        updateLDRData();
#endif
#if ENABLE_MODBUS_RTU
        updateModbusData();
#endif
#if UI_SIMULATE_RPM
        ui_update_rpm(ui_simulated_rpm());
#endif
        last_update = millis();
    }

#if defined(TOUCH_CS) && (TOUCH_CS >= 0) && TOUCH_DEBUG
    TouchDebug::poll(tft);
#endif

    delay(5);
}
