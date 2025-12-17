#include <esp_log.h>
#include <lvgl.h>
#include "display.h"

static const char *TAG = "display";

static struct panes {
	SemaphoreHandle_t semaphore;
	lv_obj_t *mainpane;
	lv_obj_t *statuspane;
} panes = {0};

void make_label(lv_obj_t *scr, lv_obj_t **lblp, int valign, int height)
{
    lv_obj_t *lbl = lv_label_create(scr);
    lv_obj_set_width(lbl, lv_pct(100));
    lv_obj_set_height(lbl, lv_pct(height));
    lv_obj_set_style_bg_color(scr, lv_color_make(0, 0, 120),
				LV_PART_MAIN);
    lv_obj_align(lbl, valign, 0, 0);

    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(lbl, lv_color_make(255, 255, 255),
				LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl, LV_ALIGN_TOP_LEFT, LV_PART_MAIN);
    lv_label_set_text(lbl, " ");
    *lblp = lbl;
}

void *init_display(lv_display_t *disp, SemaphoreHandle_t xGuiSemaphore)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_clean(scr);
    panes.semaphore = xGuiSemaphore;
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
	SemaphoreHandle_t semaphore = ((struct panes *)hdl)->semaphore;
	lv_obj_t *lbl = ((struct panes *)hdl)->mainpane;
	if (pdTRUE == xSemaphoreTake(semaphore, portMAX_DELAY)) {
		lv_obj_clean(lbl);
		lv_label_set_text(lbl, msg);
		xSemaphoreGive(semaphore);
	}
}

void draw_status(void *hdl, char *msg)
{
	SemaphoreHandle_t semaphore = ((struct panes *)hdl)->semaphore;
	lv_obj_t *lbl = ((struct panes *)hdl)->statuspane;
	if (pdTRUE == xSemaphoreTake(semaphore, portMAX_DELAY)) {
		lv_obj_clean(lbl);
		lv_label_set_text(lbl, msg);
		xSemaphoreGive(semaphore);
	}
}
