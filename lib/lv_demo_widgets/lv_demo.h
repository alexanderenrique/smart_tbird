/**
 * @file lv_demo.h
 *
 */

#ifndef LV_DEMO_H
#define LV_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

// Only include the widgets demo if available
#if LV_USE_DEMO_WIDGETS
// You must have widgets/lv_demo_widgets.h and its implementation if you enable this
#include "widgets/lv_demo_widgets.h"
#endif

/*********************
 *      DEFINES
 *********************/
// Make sure LV_USE_DEMO_WIDGETS is defined in lv_conf.h

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Call lv_demo_xxx.
 * @param   info the information which contains demo name and parameters
 *               needs by lv_demo_xxx.
 * @size    size of information.
 */
bool lv_demos_create(char * info[], int size);

/**
 * Show help for lv_demos.
 */
void lv_demos_show_help(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_DEMO_H*/