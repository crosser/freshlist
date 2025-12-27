#include <esp_log.h>
#include <lvgl.h>
#include <misc/lv_style.h>
#include "display.h"

static const char *TAG = "display";

static struct panes {
	SemaphoreHandle_t semaphore;
	struct {
		lv_obj_t *pfx;
		lv_obj_t *msg;
	} main[DISPLAY_ROWS];
	lv_obj_t *status;
	lv_obj_t *battery;
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
		LV_STYLE_CONST_WIDTH(122),
		LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(48, 64, 48)),
		LV_STYLE_CONST_BG_OPA(LV_OPA_100),
		LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_28),
		LV_STYLE_CONST_TEXT_COLOR(LV_COLOR_MAKE(160, 160, 160)),
		LV_STYLE_CONST_PROPS_END
	}));

static LV_STYLE_CONST_INIT(main_msg_style,
	((static lv_style_const_prop_t []){
		LV_STYLE_CONST_PAD_TOP(2),
		LV_STYLE_CONST_PAD_BOTTOM(2),
		LV_STYLE_CONST_PAD_LEFT(2),
		LV_STYLE_CONST_PAD_RIGHT(2),
		LV_STYLE_CONST_HEIGHT(34),
		LV_STYLE_CONST_WIDTH(412),
		LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(32, 32, 64)),
		LV_STYLE_CONST_BG_OPA(LV_OPA_100),
		LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_28),
		LV_STYLE_CONST_TEXT_COLOR(LV_COLOR_MAKE(160, 160, 160)),
		LV_STYLE_CONST_PROPS_END
	}));

static LV_STYLE_CONST_INIT(status_style,
	((static lv_style_const_prop_t []){
		LV_STYLE_CONST_PAD_TOP(2),
		LV_STYLE_CONST_PAD_BOTTOM(2),
		LV_STYLE_CONST_PAD_LEFT(2),
		LV_STYLE_CONST_PAD_RIGHT(2),
		LV_STYLE_CONST_HEIGHT(34),
		LV_STYLE_CONST_WIDTH(460),
		LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(48, 48, 48)),
		LV_STYLE_CONST_BG_OPA(LV_OPA_100),
		LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_28),
		LV_STYLE_CONST_TEXT_COLOR(LV_COLOR_MAKE(200, 200, 64)),
		LV_STYLE_CONST_PROPS_END
	}));

static LV_STYLE_CONST_INIT(battery_style,
	((static lv_style_const_prop_t []){
		LV_STYLE_CONST_PAD_TOP(2),
		LV_STYLE_CONST_PAD_BOTTOM(2),
		LV_STYLE_CONST_PAD_LEFT(2),
		LV_STYLE_CONST_PAD_RIGHT(2),
		LV_STYLE_CONST_HEIGHT(34),
		LV_STYLE_CONST_WIDTH(74),
		LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(0, 0, 0)),
		LV_STYLE_CONST_BG_OPA(LV_OPA_100),
		LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_28),
		LV_STYLE_CONST_TEXT_COLOR(LV_COLOR_MAKE(200, 200, 200)),
		LV_STYLE_CONST_PROPS_END
	}));

#if (! CONFIG_RAW_BATTERY_DISPLAY)
static void battery_draw_cb(lv_event_t * e)
{
	lv_obj_t *obj = lv_event_get_target(e);
	int value = (intptr_t)lv_obj_get_user_data(obj);
	lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
	lv_draw_dsc_base_t *base_dsc = lv_draw_task_get_draw_dsc(draw_task);
	if (base_dsc->part != LV_PART_MAIN) return;

	lv_color_t colour = (value > 0) ? lv_color_make(0, 128, 0)
					: lv_color_make(128, 0, 0);
	lv_color_t dim = lv_color_darken(colour, 50);
	lv_area_t obj_coords;
	lv_obj_get_coords(obj, &obj_coords);
	lv_area_t a = { .x1 = 2, .x2 = 60, .y1 = 2, .y2 = 26, };
	lv_area_align(&obj_coords, &a, LV_ALIGN_CENTER, 0, 0);

	lv_draw_rect_dsc_t box;
	lv_draw_rect_dsc_init(&box);
	box.border_width = 3;
	box.border_color = colour;
	box.bg_opa = LV_OPA_0;
	lv_draw_rect(base_dsc->layer, &box, &a);
	a.x1 += 2;
	a.x2 = a.x1 + (value * 56 / 100) - 4;
	a.y1 += 2;
	a.y2 -= 2;
	lv_draw_rect_dsc_t inside;
	lv_draw_rect_dsc_init(&inside);
	inside.border_width = 0;
	inside.bg_color = dim;
	lv_draw_rect(base_dsc->layer, &inside, &a);
}
#endif

void *init_display(lv_display_t *disp, SemaphoreHandle_t xGuiSemaphore)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_t *obj;
    lv_obj_add_style(scr, &screen_style, LV_PART_MAIN);
    lv_obj_clean(scr);
    panes.semaphore = xGuiSemaphore;

    for (int i = 0; i < DISPLAY_ROWS; i++) {
	obj = lv_label_create(scr);
	lv_obj_add_style(obj, &main_pfx_style, LV_PART_MAIN);
	if (i) {
		lv_obj_align_to(obj, panes.main[i-1].pfx,
				LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
	} else {
		lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 0, 0);
	}
	lv_label_set_text_static(obj, " ");
	panes.main[i].pfx = obj;

	obj = lv_label_create(scr);
	lv_obj_add_style(obj, &main_msg_style, LV_PART_MAIN);
	lv_obj_align_to(obj, panes.main[i].pfx,
				LV_ALIGN_OUT_RIGHT_MID, 2, 0);
	lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_label_set_text_static(obj, " ");
	panes.main[i].msg = obj;
    }
    obj = lv_label_create(scr);
    lv_obj_add_style(obj, &status_style, LV_PART_MAIN);
    lv_obj_align(obj, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text_static(obj, " ");
    panes.status = obj;
    obj = lv_label_create(scr);
    lv_obj_add_style(obj, &battery_style, LV_PART_MAIN);
    lv_obj_align(obj, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_label_set_text_static(obj, " ");
#if (! CONFIG_RAW_BATTERY_DISPLAY)
    lv_obj_add_event_cb(obj, battery_draw_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
#endif
    panes.battery = obj;
    return (void *)&panes;
}

void stop_display(lv_display_t *disp)
{
    lv_obj_t *scr = lv_display_get_screen_active(disp);
    lv_obj_clean(scr);
}

void draw_main(void *hdl, int row, char *prefix, char *msg)
{
	if (row >= DISPLAY_ROWS) {
		ESP_LOGI(TAG, "draw row number %d is too big: ceiling %d",
				row, DISPLAY_ROWS);
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

void draw_battery(void *hdl, int mV)
{
	ESP_LOGI(TAG, "Drawing battery %d mV", mV);
	SemaphoreHandle_t semaphore = ((struct panes *)hdl)->semaphore;
	lv_obj_t *lbl = ((struct panes *)hdl)->battery;
	if (pdTRUE == xSemaphoreTake(semaphore, portMAX_DELAY)) {
		lv_obj_clean(lbl);
#if CONFIG_RAW_BATTERY_DISPLAY
		char clevel[8];
		snprintf(clevel, sizeof(clevel), "%u", mV);
		lv_label_set_text(lbl, clevel);
#else
		int level = (mV - CONFIG_BATTERY_ADC_MIN) * 100 /
			(CONFIG_BATTERY_ADC_MAX - CONFIG_BATTERY_ADC_MIN);
		if (level < 0) level = 0;
		if (level > 100) level = 100;
		lv_obj_set_user_data(lbl, (void*)level);
		lv_obj_invalidate(lbl);
#endif
		xSemaphoreGive(semaphore);
	}
}
