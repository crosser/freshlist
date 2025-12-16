#include <esp_log.h>
#include <lvgl.h>
#include "display.h"

static const char *TAG = "display";

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

static struct panes {
	lv_obj_t *mainpane;
	lv_obj_t *statuspane;
} panes = {0};

void make_label(lv_obj_t *scr, lv_obj_t **lblp, int valign, int height)
{
    lv_obj_t *lbl = lv_label_create(scr);
    lv_obj_set_width(lbl, lv_pct(100));
    lv_obj_set_height(lbl, lv_pct(height));
    lv_obj_set_style_bg_color(scr, c_swap(lv_color_make(0, 0, 120)),
				LV_PART_MAIN);
    lv_obj_align(lbl, valign, 0, 0);

    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lbl, c_swap(lv_color_make(255, 255, 255)),
				LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl, LV_ALIGN_TOP_LEFT, LV_PART_MAIN);
    lv_label_set_text(lbl, " ");
    *lblp = lbl;
}

void *init_display(lv_display_t *disp)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_clean(scr);
    make_label(scr, &panes.mainpane, LV_ALIGN_TOP_MID, 80);
    make_label(scr, &panes.statuspane, LV_ALIGN_BOTTOM_MID, 16);
    ESP_LOGI(TAG, "returning panes pointer %p", &panes);
    return (void *)&panes;
}

void stop_display(lv_display_t *disp)
{
	return;
}

void draw_main(void *hdl, char *msg)
{
	lv_obj_t *lbl = ((struct panes *)hdl)->mainpane;
	lv_obj_clean(lbl);
	lv_label_set_text(lbl, msg);
}

void draw_status(void *hdl, char *msg)
{
	lv_obj_t *lbl = ((struct panes *)hdl)->statuspane;
	lv_obj_clean(lbl);
	lv_label_set_text(lbl, msg);
}
