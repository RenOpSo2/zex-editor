#ifndef CMD_H
#define CMD_H
#include "global.h"
enum result cmd_exec(struct global* global, struct paged_gap_buffer* cmd_buf);
#endif
