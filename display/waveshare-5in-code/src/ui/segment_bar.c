#include "segment_bar.h"

#include <stdlib.h>

#include "colors.h"

#define SEGMENT_BAR_COUNT 12
#define SEGMENT_BAR_ZONE  (SEGMENT_BAR_COUNT / 3)

typedef enum {
    SEGMENT_BAR_3_ZONE,
    SEGMENT_BAR_2_ZONE,
} segment_bar_mode_t;

typedef struct {
    segment_bar_mode_t mode;
    int32_t lo;
    int32_t mid_lo;
    int32_t mid_hi;
    int32_t hi;
    uint32_t color_lo;
    uint32_t color_mid;
    uint32_t color_hi;
} segment_bar_data_t;

#define SEGMENT_BAR_LOWER_ZONE (2 * SEGMENT_BAR_ZONE)

static void segment_bar_delete_cb(lv_event_t *e) {
    segment_bar_data_t *data = (segment_bar_data_t *)lv_event_get_user_data(e);
    free(data);
}

static uint32_t segment_color(const segment_bar_data_t *data, int index) {
    if (!data || data->mode == SEGMENT_BAR_3_ZONE) {
        if (index < SEGMENT_BAR_ZONE) {
            return common_0c9bea;
        }
        if (index < 2 * SEGMENT_BAR_ZONE) {
            return common_0cea3d;
        }
        return common_ea0c0c;
    }
    return (index < SEGMENT_BAR_LOWER_ZONE) ? data->color_lo : data->color_hi;
}

static int32_t value_to_lit_2zone(const segment_bar_data_t *data, int32_t value) {
    if (value <= data->lo) {
        return 0;
    }
    if (value >= data->hi) {
        return SEGMENT_BAR_COUNT;
    }
    if (value < data->mid_lo) {
        const int32_t span = data->mid_lo - data->lo;
        return ((value - data->lo) * SEGMENT_BAR_LOWER_ZONE) / span;
    }
    const int32_t span = data->hi - data->mid_lo;
    return SEGMENT_BAR_LOWER_ZONE + ((value - data->mid_lo) * SEGMENT_BAR_ZONE) / span;
}

static int32_t value_to_lit_3zone(const segment_bar_data_t *data, int32_t value) {
    if (value <= data->lo) {
        return 0;
    }
    if (value >= data->hi) {
        return SEGMENT_BAR_COUNT;
    }

    if (value < data->mid_lo) {
        const int32_t span = data->mid_lo - data->lo;
        return ((value - data->lo) * SEGMENT_BAR_ZONE) / span;
    }
    if (value < data->mid_hi) {
        const int32_t span = data->mid_hi - data->mid_lo;
        return SEGMENT_BAR_ZONE + ((value - data->mid_lo) * SEGMENT_BAR_ZONE) / span;
    }

    const int32_t span = data->hi - data->mid_hi;
    return (2 * SEGMENT_BAR_ZONE) + ((value - data->mid_hi) * SEGMENT_BAR_ZONE) / span;
}

static int32_t value_to_lit(const segment_bar_data_t *data, int32_t value) {
    if (data->mode == SEGMENT_BAR_2_ZONE) {
        return value_to_lit_2zone(data, value);
    }
    return value_to_lit_3zone(data, value);
}

static lv_obj_t *segment_bar_create_common(lv_obj_t *parent, segment_bar_data_t *data) {
    lv_obj_t *bar = lv_obj_create(parent);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(bar, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(bar, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(bar, lv_color_hex(common_3d3d3d), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(bar, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(bar, 20, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(bar, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(bar, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (data) {
        lv_obj_set_user_data(bar, data);
        lv_obj_add_event_cb(bar, segment_bar_delete_cb, LV_EVENT_DELETE, data);
    }

    for (int i = 0; i < SEGMENT_BAR_COUNT; i++) {
        lv_obj_t *seg = lv_obj_create(bar);
        lv_obj_clear_flag(seg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_border_width(seg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_radius(seg, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(seg, lv_color_hex(segment_color(data, i)), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(seg, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_flex_grow(seg, 1);
        lv_obj_set_height(seg, LV_PCT(100));
    }

    if (data) {
        segment_bar_set_value(bar, data->hi);
    }
    return bar;
}

lv_obj_t *segment_bar_create(lv_obj_t *parent, int32_t lo, int32_t mid_lo, int32_t mid_hi,
                             int32_t hi) {
    segment_bar_data_t *data = (segment_bar_data_t *)malloc(sizeof(segment_bar_data_t));
    if (!data) {
        return segment_bar_create_common(parent, NULL);
    }
    data->mode = SEGMENT_BAR_3_ZONE;
    data->lo = lo;
    data->mid_lo = mid_lo;
    data->mid_hi = mid_hi;
    data->hi = hi;
    return segment_bar_create_common(parent, data);
}

lv_obj_t *segment_bar_create_2zone(lv_obj_t *parent, int32_t lo, int32_t split, int32_t hi,
                                 uint32_t color_lo, uint32_t color_hi) {
    segment_bar_data_t *data = (segment_bar_data_t *)malloc(sizeof(segment_bar_data_t));
    if (!data) {
        return segment_bar_create_common(parent, NULL);
    }
    data->mode = SEGMENT_BAR_2_ZONE;
    data->lo = lo;
    data->mid_lo = split;
    data->mid_hi = split;
    data->hi = hi;
    data->color_lo = color_lo;
    data->color_hi = color_hi;
    return segment_bar_create_common(parent, data);
}

void segment_bar_set_value(lv_obj_t *bar, int32_t value) {
    if (!bar) {
        return;
    }

    segment_bar_data_t *data = (segment_bar_data_t *)lv_obj_get_user_data(bar);
    if (!data) {
        return;
    }
    if (data->mode == SEGMENT_BAR_3_ZONE &&
        !(data->lo < data->mid_lo && data->mid_lo < data->mid_hi && data->mid_hi < data->hi)) {
        return;
    }
    if (data->mode == SEGMENT_BAR_2_ZONE && !(data->lo < data->mid_lo && data->mid_lo < data->hi)) {
        return;
    }

    if (value < data->lo) {
        value = data->lo;
    } else if (value > data->hi) {
        value = data->hi;
    }

    const int32_t lit = value_to_lit(data, value);
    const uint32_t child_cnt = lv_obj_get_child_cnt(bar);

    for (uint32_t i = 0; i < child_cnt && i < SEGMENT_BAR_COUNT; i++) {
        lv_obj_t *seg = lv_obj_get_child(bar, i);
        if (!seg) {
            continue;
        }
        const lv_opa_t opa = ((int32_t)i < lit) ? LV_OPA_COVER : LV_OPA_TRANSP;
        lv_obj_set_style_bg_opa(seg, opa, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}
