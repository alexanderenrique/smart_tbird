#pragma once

// XPT2046 touch diagnostics (aligned with NEMO Tool Display /display-firmware).
// Uses ESP_LOGI so output is visible with display_n8 monitor_filters.

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <esp_log.h>
#include <stdarg.h>

#ifndef TOUCH_DEBUG
#define TOUCH_DEBUG 0
#endif

namespace TouchDebug {

static const char *TAG = "touch";

inline void log(const char *fmt, ...) {
    char buf[240];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    ESP_LOGI(TAG, "%s", buf);
    Serial.println(buf);
    Serial.flush();
}

#if defined(TOUCH_CS) && (TOUCH_CS >= 0)

inline void logPinConfig() {
    log("=== Touch / SPI pin config (Thunderbird) ===");
    log("  TOUCH_CS (T_CS)  GPIO %d", TOUCH_CS);
#ifdef TFT_SCLK
    log("  TFT_SCLK (T_CLK) GPIO %d", TFT_SCLK);
#endif
#ifdef TFT_MOSI
    log("  TFT_MOSI (T_DIN) GPIO %d", TFT_MOSI);
#endif
#ifdef TFT_MISO
    log("  TFT_MISO (T_DO)  GPIO %d", TFT_MISO);
#else
    log("  TFT_MISO (T_DO)  *** NOT DEFINED ***");
    log("    -> TFT_eSPI aliases MISO=MOSI on ESP32-S3; touch will not work without T_DO wired.");
#endif
#ifdef TFT_CS
    log("  TFT_CS           GPIO %d", TFT_CS);
#endif
#ifdef SPI_TOUCH_FREQUENCY
    log("  SPI_TOUCH_FREQ   %lu Hz", (unsigned long)SPI_TOUCH_FREQUENCY);
#endif

#if defined(TFT_MISO) && defined(TFT_MOSI)
    if (TFT_MISO == TFT_MOSI) {
        log("*** WARN: TFT_MISO == TFT_MOSI — T_DO must be its own GPIO ***");
    }
#endif

    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
#ifdef TFT_MISO
    pinMode(TFT_MISO, INPUT);
    log("  T_DO digitalRead=%d (4095 on SPI often reads as Z=0 in TFT_eSPI)", digitalRead(TFT_MISO));
#endif
    log("=== end touch pin config ===");
}

inline void logRawSample(TFT_eSPI &tft, const char *tag) {
    uint16_t raw_x = 0;
    uint16_t raw_y = 0;
    const uint16_t raw_z = tft.getTouchRawZ();
    tft.getTouchRaw(&raw_x, &raw_y);

    log("[%s] raw X=%u Y=%u Z=%u", tag, raw_x, raw_y, raw_z);

    if (raw_z == 0) {
        log("  Z=0: TFT_eSPI maps SPI 0xFFF->0; often means T_DO stuck HIGH/floating, not returning data");
    } else if (raw_z < 350) {
        log("  Z low — finger likely not detected (threshold ~350-600)");
    } else {
        log("  Z high — pressure detected");
    }

    uint16_t cal_x = 0;
    uint16_t cal_y = 0;
    const bool touched = tft.getTouch(&cal_x, &cal_y, TOUCH_PRESSURE_THRESHOLD);
    log("  getTouch(th=%d)=%s", TOUCH_PRESSURE_THRESHOLD, touched ? "YES" : "no");
    if (touched) {
        log("  calibrated (%u,%u)", cal_x, cal_y);
    }
}

inline void poll(TFT_eSPI &tft) {
    static unsigned long last_ms = 0;
    static uint32_t poll_count = 0;
    static uint16_t last_z = 0;

    const unsigned long now = millis();
    if (now - last_ms < 1000) {
        return;
    }
    last_ms = now;
    poll_count++;

    uint16_t raw_x = 0;
    uint16_t raw_y = 0;
    const uint16_t raw_z = tft.getTouchRawZ();
    tft.getTouchRaw(&raw_x, &raw_y);

    uint16_t cal_x = 0;
    uint16_t cal_y = 0;
    const bool pressed = tft.getTouch(&cal_x, &cal_y, TOUCH_PRESSURE_THRESHOLD);

#ifdef TFT_MISO
    const int tdo_level = digitalRead(TFT_MISO);
#else
    const int tdo_level = -1;
#endif

  // Always print Z — this is the line you want on serial/monitor
    log("touch#%lu Z=%u raw(%u,%u) T_DO=%d getTouch=%s",
        poll_count, raw_z, raw_x, raw_y, tdo_level, pressed ? "YES" : "no");

    if (pressed) {
        log("  -> cal(%u,%u)", cal_x, cal_y);
    }

    if (raw_z == 0 && last_z == 0 && poll_count <= 3) {
        log("  hint: scope activity on GPIO 18 without Z>0 usually means analog pickup, not valid SPI MISO");
    }

    if (raw_z > last_z + 80 || (last_z > 80 && raw_z < last_z - 80)) {
        log("  ** Z changed %u -> %u (press/release?) **", last_z, raw_z);
    }

    last_z = raw_z;
}

inline void logPressDetail(TFT_eSPI &tft, uint16_t cal_x, uint16_t cal_y, int lvgl_x, int lvgl_y) {
    uint16_t raw_x = 0;
    uint16_t raw_y = 0;
    const uint16_t raw_z = tft.getTouchRawZ();
    tft.getTouchRaw(&raw_x, &raw_y);
    log("Touch EVENT raw(%u,%u) Z=%u -> cal(%u,%u) -> LVGL(%d,%d)",
        raw_x, raw_y, raw_z, cal_x, cal_y, lvgl_x, lvgl_y);
}

#else  // TOUCH_CS

inline void logPinConfig() {
    log("TOUCH_CS not defined — touch disabled at compile time");
}

inline void poll(TFT_eSPI &) {}
inline void logRawSample(TFT_eSPI &, const char *) {}
inline void logPressDetail(TFT_eSPI &, uint16_t, uint16_t, int, int) {}

#endif  // TOUCH_CS

}  // namespace TouchDebug
