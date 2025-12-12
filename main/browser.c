/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <esp_log.h>
#include <lvgl.h>
#include "browser.h"

static const char *TAG = "browser";

/* Create a pseudo lv_color_t that will produce byte-swapped r5g6b5 */
static lv_color_t c_swap(lv_color_t o)
{
    /*
       RRRrr... GGGggg.. BBbbb...
        ___  __ __  ___
       /   \/     \/   \
       RRRrrGGG gggBBbbb
               X
       gggBBbbb RRRrrGGG
       \___/\__ __/\___/
       r     g-     b---
       |       \        \
       gggBB000 bbbRRR00 rrGGG000
     */
    return (lv_color_t) {
	.red =   ((o.green << 3) & 0b11100000) | ((o.blue >> 3)  & 0b00011000),
	.green = ((o.blue << 2)  & 0b11100000) | ((o.red >> 3)   & 0b00011100),
	.blue =  ((o.red << 3)   & 0b11000000) | ((o.green >> 2) & 0b00111000)
    };
}

static lv_obj_t *label;

void *init_browser(lv_display_t *disp)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    label = lv_label_create(scr);
    lv_obj_set_width(label, lv_pct(96));
    lv_obj_set_height(label, lv_pct(90));
    lv_obj_set_style_bg_color(scr, c_swap(lv_color_make(0, 0, 120)),
				LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);

    lv_label_set_text(label, "Hello Espressif, Hello LVGL. "
		      "This is an especially long text that is expected "
		      "to scroll circularly");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label, c_swap(lv_color_make(255, 255, 255)),
				LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_ALIGN_TOP_LEFT, LV_PART_MAIN);
    ESP_LOGI(TAG, "returning label pointer %p", label);
    return (void *)label;
}

void stop_browser(lv_display_t *disp)
{
	return;
}

void draw(void *hdl, char *msg)
{
	lv_obj_t *lbl = (lv_obj_t *)hdl;
	lv_obj_clean(label);
	lv_label_set_text(lbl, msg);
}
