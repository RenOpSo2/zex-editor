#include "render_buffer.h"

#include <string.h>
#include <unistd.h>


void rb_init(RenderBuffer *rb)
{
    rb->len = 0;
}

void rb_clear(RenderBuffer *rb)
{
    rb->len = 0;
}

void rb_append(RenderBuffer *rb, const void *src, size_t size)
{
    if (!rb || !src || size == 0)
        return;

    // Check if adding this would overflow the buffer
    if (rb->len + size > buf_capacity) {
        // Truncate to fit available space
        size = buf_capacity - rb->len;
    }

    if (size == 0)
        return;

    memcpy(rb->data + rb->len, src, size);
    rb->len += size;
}

void rb_append_char(RenderBuffer *rb, char ch)
{
    if (!rb)
        return;

    if (rb->len >= buf_capacity)
        return;

    rb->data[rb->len++] = ch;
}

void rb_append_str(RenderBuffer *rb, const char *str)
{
    if (!str)
        return;

    rb_append(rb, str, strlen(str));
}

void rb_flush(RenderBuffer *rb)
{
    if (!rb)
        return;

    if (rb->len == 0)
        return;

    write(STDOUT_FILENO, rb->data, rb->len);

    rb->len = 0;
}
