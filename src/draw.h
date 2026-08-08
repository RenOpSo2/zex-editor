#ifndef DRAW_H
#define DRAW_H
#include "global.h"
void draw_init(void);
void draw_update(struct global* global);
void draw_deinit(void);
uint32_t draw_get_scroll_offset(void);
#endif
