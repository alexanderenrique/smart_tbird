#ifndef SEGMENT_BAR_H
#define SEGMENT_BAR_H

#include <lvgl.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Outlined track with 12 vertical segments (4 blue / 4 green / 4 red).
 * Value maps piecewise: lo→mid_lo (blue), mid_lo→mid_hi (green), mid_hi→hi (red). */
lv_obj_t *segment_bar_create(lv_obj_t *parent, int32_t lo, int32_t mid_lo, int32_t mid_hi,
                             int32_t hi);

/* Two-zone bar: lower 8 segments (color_lo), upper 4 segments (color_hi).
 * Value maps linearly: lo→split (lower zone), split→hi (upper zone). */
lv_obj_t *segment_bar_create_2zone(lv_obj_t *parent, int32_t lo, int32_t split, int32_t hi,
                                   uint32_t color_lo, uint32_t color_hi);

void segment_bar_set_value(lv_obj_t *bar, int32_t value);

#ifdef __cplusplus
}
#endif

#endif /* SEGMENT_BAR_H */
