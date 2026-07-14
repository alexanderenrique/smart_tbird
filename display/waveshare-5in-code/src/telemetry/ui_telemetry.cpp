#include "ui_telemetry.h"

#include <stdio.h>
#include <math.h>

#include <lvgl.h>

#include "ui/screens.h"

static int32_t clamp_i32(int32_t value, int32_t lo, int32_t hi) {
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

static void set_temp_label(lv_obj_t *label, float temp_f) {
    if (label == nullptr) {
        return;
    }
    char buf[24];
    snprintf(buf, sizeof(buf), "%.0f F", static_cast<double>(temp_f));
    lv_label_set_text(label, buf);
}

void ui_apply_telemetry(const TelemetryData &data) {
    if (!data.valid) {
        return;
    }

    char buf[24];

    if (objects.voltage_value_label) {
        snprintf(buf, sizeof(buf), "%.1fV", static_cast<double>(data.voltage_v));
        lv_label_set_text(objects.voltage_value_label, buf);
    }
    set_temp_label(objects.oil_value_label, data.oil_temp_f);
    set_temp_label(objects.coolant_value_label, data.coolant_temp_f);
    set_temp_label(objects.trans_value_label, data.trans_temp_f);

    if (objects.afr_value_label) {
        snprintf(buf, sizeof(buf), "%.1f", static_cast<double>(data.afr));
        lv_label_set_text(objects.afr_value_label, buf);
    }

    if (objects.rpm_value_label) {
        snprintf(buf, sizeof(buf), "%u", static_cast<unsigned>(data.rpm));
        lv_label_set_text(objects.rpm_value_label, buf);
    }

    // Screen 2: label_3 is the PCB numeric value.
    set_temp_label(objects.label_3, data.pcb_temp_f);
    set_temp_label(objects.iat_value_label, data.iat_f);

    if (objects.fan_pwm_value_label) {
        snprintf(buf, sizeof(buf), "%u %%", static_cast<unsigned>(data.fan_pwm_pct));
        lv_label_set_text(objects.fan_pwm_value_label, buf);
    }

    // Voltage bar uses hundredths of a volt (10.0–16.0 V → 1000–1600).
    if (objects.voltage_bar) {
        const int32_t v = clamp_i32(static_cast<int32_t>(lroundf(data.voltage_v * 100.0f)), 1000, 1600);
        lv_bar_set_value(objects.voltage_bar, v, LV_ANIM_OFF);
    }
    if (objects.oil_bar) {
        lv_bar_set_value(objects.oil_bar, clamp_i32(lroundf(data.oil_temp_f), 50, 250), LV_ANIM_OFF);
    }
    if (objects.coolant_bar) {
        lv_bar_set_value(objects.coolant_bar, clamp_i32(lroundf(data.coolant_temp_f), 50, 250), LV_ANIM_OFF);
    }
    if (objects.trans_bar) {
        lv_bar_set_value(objects.trans_bar, clamp_i32(lroundf(data.trans_temp_f), 50, 250), LV_ANIM_OFF);
    }
    if (objects.bar_1) {
        lv_bar_set_value(objects.bar_1, clamp_i32(data.rpm, 0, 6000), LV_ANIM_OFF);
    }
    if (objects.afr_arc) {
        // Arc range is AFR tenths (100–180 for 10.0–18.0).
        const int32_t afr_tenths = clamp_i32(lroundf(data.afr * 10.0f), 100, 180);
        lv_arc_set_value(objects.afr_arc, afr_tenths);
    }
    if (objects.pcb_temp_bar) {
        lv_bar_set_value(objects.pcb_temp_bar, clamp_i32(lroundf(data.pcb_temp_f), 25, 120), LV_ANIM_OFF);
    }
    if (objects.iat_temp_bar) {
        lv_bar_set_value(objects.iat_temp_bar, clamp_i32(lroundf(data.iat_f), 25, 200), LV_ANIM_OFF);
    }
    if (objects.bar_2) {
        lv_bar_set_value(objects.bar_2, clamp_i32(data.fan_pwm_pct, 0, 100), LV_ANIM_OFF);
    }

    if (objects.voltage_value_label && objects.voltage_bar) {
        lv_obj_align_to(objects.voltage_value_label, objects.voltage_bar, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    }
    if (objects.oil_value_label && objects.oil_bar) {
        lv_obj_align_to(objects.oil_value_label, objects.oil_bar, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    }
    if (objects.coolant_value_label && objects.coolant_bar) {
        lv_obj_align_to(objects.coolant_value_label, objects.coolant_bar, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    }
    if (objects.trans_value_label && objects.trans_bar) {
        lv_obj_align_to(objects.trans_value_label, objects.trans_bar, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    }
    if (objects.label_3 && objects.pcb_temp_bar) {
        lv_obj_align_to(objects.label_3, objects.pcb_temp_bar, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    }
    if (objects.iat_value_label && objects.iat_temp_bar) {
        lv_obj_align_to(objects.iat_value_label, objects.iat_temp_bar, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    }
    if (objects.fan_pwm_value_label && objects.bar_2) {
        lv_obj_align_to(objects.fan_pwm_value_label, objects.bar_2, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    }
    ui_align_afr_readout();
}
