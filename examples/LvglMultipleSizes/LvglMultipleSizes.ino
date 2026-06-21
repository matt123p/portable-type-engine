#include <lvgl.h>
#include <lv_pte.h>
#include "pte_demo_font.h"

static lv_font_t * fonts[3];

static void create_demo(void)
{
    static const int32_t sizes[] = {14, 28, 42};
    lv_obj_t * cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(cont, LV_PCT(90), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_center(cont);

    for(uint32_t i = 0; i < 3; i++) {
        fonts[i] = lv_pte_create(get_pte_demo_font(), sizes[i]);
        lv_obj_t * label = lv_label_create(cont);
        lv_obj_set_style_text_font(label, fonts[i], 0);
        lv_label_set_text(label, "PTE");
    }
}

void setup(void)
{
    lv_init();
    /* Initialize your LVGL display driver here. */
    if(lv_display_get_default() != NULL) create_demo();
}

void loop(void)
{
    lv_timer_handler();
    delay(5);
}
