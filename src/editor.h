#ifndef EDITOR_H
#define EDITOR_H
#include "global.h"
void editor_init(struct global* global, const char* filepath);
void editor_deinit(struct global* global);
enum result editor_update(struct global* global);
#endif
