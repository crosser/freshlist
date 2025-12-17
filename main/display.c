#include <esp_log.h>
#include <lvgl.h>
#include <misc/lv_style.h>
#include "display.h"

static const char *TAG = "display";

#define ROWS 6

static struct panes {
	SemaphoreHandle_t semaphore;
	struct {
		lv_obj_t *pfx;
		lv_obj_t *msg;
	} main[ROWS];
	lv_obj_t *status;
} panes = {0};

static LV_STYLE_CONST_INIT(screen_style,
	((static lv_style_const_prop_t []){
		LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(0, 0, 0)),
		LV_STYLE_CONST_BG_OPA(LV_OPA_100),
	}));

static LV_STYLE_CONST_INIT(main_pfx_style,
	((static lv_style_const_prop_t []){
		LV_STYLE_CONST_PAD_TOP(2),
		LV_STYLE_CONST_PAD_BOTTOM(2),
		LV_STYLE_CONST_PAD_LEFT(2),
		LV_STYLE_CONST_PAD_RIGHT(2),
		LV_STYLE_CONST_HEIGHT(34),
		LV_STYLE_CONST_WIDTH(107),
		LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(0, 64, 0)),
		LV_STYLE_CONST_BG_OPA(LV_OPA_100),
		LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_28),
		LV_STYLE_CONST_TEXT_COLOR(LV_COLOR_MAKE(127, 127, 127)),
		LV_STYLE_CONST_PROPS_END
	}));

static LV_STYLE_CONST_INIT(main_msg_style,
	((static lv_style_const_prop_t []){
		LV_STYLE_CONST_PAD_TOP(2),
		LV_STYLE_CONST_PAD_BOTTOM(2),
		LV_STYLE_CONST_PAD_LEFT(2),
		LV_STYLE_CONST_PAD_RIGHT(2),
		LV_STYLE_CONST_HEIGHT(34),
		LV_STYLE_CONST_WIDTH(536),
		LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(0, 0, 64)),
		LV_STYLE_CONST_BG_OPA(LV_OPA_100),
		LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_28),
		LV_STYLE_CONST_TEXT_COLOR(LV_COLOR_MAKE(127, 127, 127)),
		LV_STYLE_CONST_PROPS_END
	}));

static LV_STYLE_CONST_INIT(status_style,
	((static lv_style_const_prop_t []){
		LV_STYLE_CONST_PAD_TOP(2),
		LV_STYLE_CONST_PAD_BOTTOM(2),
		LV_STYLE_CONST_PAD_LEFT(2),
		LV_STYLE_CONST_PAD_RIGHT(2),
		LV_STYLE_CONST_HEIGHT(34),
		LV_STYLE_CONST_WIDTH(536),
		LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(64, 64, 64)),
		LV_STYLE_CONST_BG_OPA(LV_OPA_100),
		LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_28),
		LV_STYLE_CONST_TEXT_COLOR(LV_COLOR_MAKE(127, 127, 127)),
		LV_STYLE_CONST_PROPS_END
	}));

void *init_display(lv_display_t *disp, SemaphoreHandle_t xGuiSemaphore)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_t *obj;
    lv_obj_add_style(scr, &screen_style, LV_PART_MAIN);
    lv_obj_clean(scr);
    panes.semaphore = xGuiSemaphore;

    for (int i = 0; i < ROWS; i++) {
	obj = lv_label_create(scr);
	lv_obj_add_style(obj, &main_pfx_style, LV_PART_MAIN);
	if (i) {
		lv_obj_align_to(obj, panes.main[i-1].pfx,
				LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
	} else {
		lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, 0);
	}
	lv_label_set_text_static(obj, "pfx");
	panes.main[i].pfx = obj;

	obj = lv_label_create(scr);
	lv_obj_add_style(obj, &main_msg_style, LV_PART_MAIN);
	lv_obj_align_to(obj, panes.main[i].pfx,
				LV_ALIGN_OUT_RIGHT_MID, 0, 0);
	lv_label_set_text_static(obj, "msg");
	panes.main[i].msg = obj;
    }
    obj = lv_label_create(scr);
    lv_obj_add_style(obj, &status_style, LV_PART_MAIN);
    lv_obj_align(obj, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
    panes.status = obj;
    return (void *)&panes;
}

void stop_display(lv_display_t *disp)
{
	return;
}

void draw_main(void *hdl, int row, char *prefix, char *msg)
{
	if (row >= ROWS) {
		ESP_LOGI(TAG, "draw row number %d is too big: ceiling %d",
				row, ROWS);
		return;
	}
	SemaphoreHandle_t semaphore = ((struct panes *)hdl)->semaphore;
	if (pdTRUE == xSemaphoreTake(semaphore, portMAX_DELAY)) {
		lv_obj_t *lbl = ((struct panes *)hdl)->main[row].pfx;
		lv_obj_clean(lbl);
		lv_label_set_text(lbl, prefix);
		lbl = ((struct panes *)hdl)->main[row].msg;
		lv_obj_clean(lbl);
		lv_label_set_text(lbl, msg);
		xSemaphoreGive(semaphore);
	}
}

void draw_status(void *hdl, char *msg)
{
	SemaphoreHandle_t semaphore = ((struct panes *)hdl)->semaphore;
	lv_obj_t *lbl = ((struct panes *)hdl)->status;
	if (pdTRUE == xSemaphoreTake(semaphore, portMAX_DELAY)) {
		lv_obj_clean(lbl);
		lv_label_set_text(lbl, msg);
		xSemaphoreGive(semaphore);
	}
}
