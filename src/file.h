#ifndef FILE_H
#define FILE_H
#include "global.h"
enum result file_read(struct paged_gap_buffer* pgb, const char* path, Arena* arena);
enum result file_write(const char* path, struct paged_gap_buffer* pgb);
#endif
