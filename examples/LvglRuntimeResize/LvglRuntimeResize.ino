#include <lvgl.h>
#include <lv_pte.h>
#include "pte_demo_font.h"

static lv_font_t * font;
static lv_style_t text_style;

static void size_changed(lv_event_t * event)
{
    lv_obj_t * slider = lv_event_get_target_obj(event);
    lv_pte_set_size(font, lv_slider_get_value(slider));
    lv_obj_report_style_change(&text_style);
}

static void create_demo(void)
{
    font = lv_pte_create(get_pte_demo_font(), 28);
    lv_style_init(&text_style);
    lv_style_set_text_font(&text_style, font);

    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_obj_add_style(label, &text_style, 0);
    lv_label_set_text(label, "PTE");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t * slider = lv_slider_create(lv_screen_active());
    lv_slider_set_range(slider, 10, 60);
    lv_slider_set_value(slider, 28, LV_ANIM_OFF);
    lv_obj_set_width(slider, LV_PCT(70));
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 50);
    lv_obj_add_event_cb(slider, size_changed, LV_EVENT_VALUE_CHANGED, NULL);
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
