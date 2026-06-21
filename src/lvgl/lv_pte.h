/*
 * Portable Type Engine adapter for LVGL 9.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LV_PTE_H
#define LV_PTE_H

#include <lvgl.h>
#include "../pte/pte.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LVGL_VERSION_MAJOR > 9 || (LVGL_VERSION_MAJOR == 9 && LVGL_VERSION_MINOR >= 5)
/** PTE font class for lv_font_manager_add_src_static() on LVGL 9.5+. */
LV_ATTRIBUTE_EXTERN_DATA extern const lv_font_class_t lv_pte_font_class;
#endif

/** Create an LVGL font at font_size pixels from generated PTE font data. */
lv_font_t * lv_pte_create(const pte_base_font * src, int32_t font_size);

/** Change the render size of a font created by lv_pte_create(). */
void lv_pte_set_size(lv_font_t * font, int32_t font_size);

/** Destroy a font created by lv_pte_create(). */
void lv_pte_destroy(lv_font_t * font);

#ifdef __cplusplus
}
#endif

#endif
