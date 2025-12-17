#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <freertos/FreeRTOS.h>
#include <lvgl.h>

void *init_display(lv_display_t *disp, SemaphoreHandle_t xGuiSemaphore);
void stop_display(lv_display_t *disp);
void draw_main(void *hdl, char *msg);
void draw_status(void *hdl, char *msg);

#endif
