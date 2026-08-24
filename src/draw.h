#ifndef DRAW_H
#define DRAW_H
#include "global.h"
void draw_init(void);
void draw_update(struct global* global);
void draw_deinit(void);
uint32_t draw_get_scroll_offset(void);

/** Columns occupied by the line-number gutter (0 when disabled). */
uint32_t draw_gutter_width(void);
#endif
