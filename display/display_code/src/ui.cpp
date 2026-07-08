#include "ui.h"

#include <Arduino.h>
#include <lvgl.h>
#include <esp_log.h>
#include <stdio.h>

static constexpr const char *UI_TAG = "ui";

static constexpr int DISPLAY_W = 320;
static constexpr int DISPLAY_H = 480;

static constexpr uint16_t RPM_MAX = 6000;
static constexpr int SEGMENT_COUNT = 40;
static constexpr int RPM_PER_SEGMENT = 150;
static constexpr int MIN_BAR_WIDTH = 48;
static constexpr int SEGMENT_GAP = 1;
static constexpr int RPM_LABEL_AREA = 60;

static const lv_color_t BACKGROUND_COLOR = lv_color_hex(0x191970);
static const lv_color_t TEXT_COLOR = lv_color_hex(0xFFFFFF);
static const lv_color_t TRACK_COLOR = lv_color_hex(0x333355);
static const lv_color_t LIT_COLOR = lv_color_hex(0xFF4500);

static const lv_font_t *TITLE_FONT = &lv_font_montserrat_24;
static const lv_font_t *LARGE_FONT = &lv_font_montserrat_38;

static lv_obj_t *oil_label;
static lv_obj_t *coolant_label;
static lv_obj_t *trans_label;
static lv_obj_t *o2_label;
static lv_obj_t *voltage_label;
static lv_obj_t *brightness_label;
static lv_obj_t *rpm_label;
static lv_obj_t *segment_bars[SEGMENT_COUNT];
static int segment_max_w[SEGMENT_COUNT];

static int segment_max_width(int index) {
    if (index <= 0) return MIN_BAR_WIDTH;
    if (index >= SEGMENT_COUNT - 1) return DISPLAY_W;
    return MIN_BAR_WIDTH + (DISPLAY_W - MIN_BAR_WIDTH) * index / (SEGMENT_COUNT - 1);
}

static void allow_parent_swipe(lv_obj_t *obj) {
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
}

static lv_obj_t *create_value_label(lv_obj_t *parent, int y) {
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, LARGE_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 20, y);
    allow_parent_swipe(label);
    return label;
}

static void style_page_bg(lv_obj_t *obj) {
    lv_obj_set_style_bg_color(obj, BACKGROUND_COLOR, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
}

static void create_dashboard_tile(lv_obj_t *tile) {
    style_page_bg(tile);
    allow_parent_swipe(tile);

    lv_obj_t *title = lv_label_create(tile);
    lv_label_set_text(title, "Alex's Thunderbird");
    lv_obj_set_style_text_font(title, TITLE_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    allow_parent_swipe(title);

    oil_label = create_value_label(tile, 60);
    coolant_label = create_value_label(tile, 110);
    trans_label = create_value_label(tile, 160);
    o2_label = create_value_label(tile, 210);
    voltage_label = create_value_label(tile, 260);

    brightness_label = lv_label_create(tile);
    lv_label_set_text(brightness_label, "Brt: --%");
    lv_obj_set_style_text_font(brightness_label, TITLE_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(brightness_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 20, 440);
    allow_parent_swipe(brightness_label);
}

static void create_rpm_tile(lv_obj_t *tile) {
    style_page_bg(tile);
    allow_parent_swipe(tile);

    const int segment_area_h = DISPLAY_H - RPM_LABEL_AREA;
    const int segment_h = (segment_area_h - (SEGMENT_COUNT - 1) * SEGMENT_GAP) / SEGMENT_COUNT;

    lv_obj_t *segments = lv_obj_create(tile);
    lv_obj_set_size(segments, DISPLAY_W, segment_area_h);
    lv_obj_align(segments, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(segments, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(segments, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(segments, 0, LV_PART_MAIN);
    allow_parent_swipe(segments);

    for (int i = 0; i < SEGMENT_COUNT; i++) {
        const int max_w = segment_max_width(i);
        segment_max_w[i] = max_w;
        const int y = segment_area_h - (i + 1) * segment_h - i * SEGMENT_GAP;

        lv_obj_t *bar = lv_bar_create(segments);
        segment_bars[i] = bar;
        lv_bar_set_range(bar, 0, max_w);
        lv_obj_set_size(bar, max_w, segment_h);
        lv_obj_set_pos(bar, DISPLAY_W - max_w, y);
        lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 0, LV_PART_INDICATOR);
        lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, TRACK_COLOR, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_bg_color(bar, LIT_COLOR, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
        lv_obj_set_style_base_dir(bar, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_bar_set_value(bar, 0, LV_ANIM_OFF);
        allow_parent_swipe(bar);
    }

    rpm_label = lv_label_create(tile);
    lv_label_set_text(rpm_label, "0");
    lv_obj_set_style_text_font(rpm_label, LARGE_FONT, LV_PART_MAIN);
    lv_obj_set_style_text_color(rpm_label, TEXT_COLOR, LV_PART_MAIN);
    lv_obj_align(rpm_label, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    allow_parent_swipe(rpm_label);
}

void ui_init(void) {
    ESP_LOGI(UI_TAG, "tileview create");
    lv_obj_t *scr = lv_scr_act();
    style_page_bg(scr);

    lv_obj_t *tileview = lv_tileview_create(scr);
    lv_obj_set_size(tileview, DISPLAY_W, DISPLAY_H);
    style_page_bg(tileview);
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

    ESP_LOGI(UI_TAG, "add tiles");
    lv_obj_t *dashboard_tile = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR);
    lv_obj_t *rpm_tile = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR);

    ESP_LOGI(UI_TAG, "dashboard tile");
    create_dashboard_tile(dashboard_tile);

    ESP_LOGI(UI_TAG, "rpm tile");
    create_rpm_tile(rpm_tile);

    ui_set_dashboard_placeholders();
    ESP_LOGI(UI_TAG, "init done");
}

void ui_set_dashboard_placeholders(void) {
    if (!oil_label) return;

    lv_label_set_text(oil_label, "Oil: --");
    lv_label_set_text(coolant_label, "Coolant: --");
    lv_label_set_text(trans_label, "Trans: --");
    lv_label_set_text(o2_label, "O2: --");
    lv_label_set_text(voltage_label, "Voltage: --");
}

void ui_update_dashboard(const UiDashboardData *data) {
    if (!oil_label) return;

    if (!data || !data->valid) {
        ui_set_dashboard_placeholders();
        return;
    }

    char text[32];

    snprintf(text, sizeof(text), "Oil: %.1f°F", data->oil_temp_f);
    lv_label_set_text(oil_label, text);

    snprintf(text, sizeof(text), "Coolant: %.1f°F", data->coolant_temp_f);
    lv_label_set_text(coolant_label, text);

    snprintf(text, sizeof(text), "Trans: %.1f°F", data->trans_temp_f);
    lv_label_set_text(trans_label, text);

    snprintf(text, sizeof(text), "O2: %.2f", data->o2_afr);
    lv_label_set_text(o2_label, text);

    snprintf(text, sizeof(text), "Voltage: %.2f V", data->voltage_v);
    lv_label_set_text(voltage_label, text);
}

void ui_update_brightness_pct(int pct) {
    if (!brightness_label) return;

    char text[24];
    snprintf(text, sizeof(text), "Brt: %d%%", pct);
    lv_label_set_text(brightness_label, text);
}

void ui_update_rpm(uint16_t rpm) {
    if (!rpm_label) return;

    if (rpm > RPM_MAX) rpm = RPM_MAX;

    char text[16];
    snprintf(text, sizeof(text), "%u", rpm);
    lv_label_set_text(rpm_label, text);

    const int full_segments = rpm / RPM_PER_SEGMENT;
    const int remainder = rpm % RPM_PER_SEGMENT;

    for (int i = 0; i < SEGMENT_COUNT; i++) {
        lv_obj_t *bar = segment_bars[i];
        if (!bar) continue;

        int fill_w = 0;
        if (i < full_segments) {
            fill_w = segment_max_w[i];
        } else if (i == full_segments && remainder > 0) {
            fill_w = (segment_max_w[i] * remainder) / RPM_PER_SEGMENT;
        }

        lv_bar_set_value(bar, fill_w, LV_ANIM_OFF);
    }
}

#if UI_SIMULATE_RPM
uint16_t ui_simulated_rpm(void) {
    static constexpr unsigned long HALF_PERIOD_MS = 6000;
    const unsigned long cycle_ms = HALF_PERIOD_MS * 2;
    const unsigned long phase_ms = millis() % cycle_ms;

    if (phase_ms < HALF_PERIOD_MS) {
        return (uint16_t)((unsigned long)RPM_MAX * phase_ms / HALF_PERIOD_MS);
    }

    const unsigned long down_ms = phase_ms - HALF_PERIOD_MS;
    return (uint16_t)(RPM_MAX - ((unsigned long)RPM_MAX * down_ms / HALF_PERIOD_MS));
}
#endif
