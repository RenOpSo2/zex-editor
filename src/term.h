#ifndef TERM_H
#define TERM_H

#include "global.h"

void term_init(void);
void term_deinit(void);
uint32_t term_read(char* dst);
void term_update(struct term* term);

#endif
