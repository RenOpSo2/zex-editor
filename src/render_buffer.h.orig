#ifndef RENDER_BUFFER_H
#define RENDER_BUFFER_H

#include "global.h"
#include <stddef.h>

typedef struct {
    char data[buf_capacity];
    size_t len;
} RenderBuffer;

void rb_init(RenderBuffer *rb);
void rb_clear(RenderBuffer *rb);

void rb_append(RenderBuffer *rb, const void *src, size_t size);
void rb_append_char(RenderBuffer *rb, char ch);
void rb_append_str(RenderBuffer *rb, const char *str);

void rb_flush(RenderBuffer *rb);

#endif
