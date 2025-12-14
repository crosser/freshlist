#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <lvgl.h>

void *init_display(lv_display_t *disp);
void stop_display(lv_display_t *disp);
void draw(void *hdl, char *msg);

#endif
