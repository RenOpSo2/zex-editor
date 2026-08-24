#include "render_buffer.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/* Initial capacity covers a typical frame; growth is geometric afterwards. */
#define RB_INITIAL_CAPACITY 4096u

/* Ceiling so a pathological frame can never exhaust the machine's memory. */
#define RB_MAX_CAPACITY ((size_t)64 * 1024 * 1024)

static enum result rb_grow(RenderBuffer* rb, size_t needed)
{
    if (needed <= rb->cap) {
        return ok;
    }

    size_t new_cap = rb->cap ? rb->cap : RB_INITIAL_CAPACITY;
    while (new_cap < needed && new_cap <= RB_MAX_CAPACITY / 2) {
        new_cap *= 2;
    }
    if (new_cap < needed) {
        /* Doubling was capped; honour the exact request if it fits. */
        new_cap = needed;
    }
    if (new_cap > RB_MAX_CAPACITY) {
        return err;
    }

    char* grown = realloc(rb->data, new_cap);
    if (!grown) {
        return err;
    }

    rb->data = grown;
    rb->cap = new_cap;
    return ok;
}

void rb_init(RenderBuffer* rb)
{
    if (!rb) return;
    rb->len = 0;
    rb->overflowed = false;
}

void rb_deinit(RenderBuffer* rb)
{
    if (!rb) return;
    free(rb->data);
    rb->data = NULL;
    rb->len = 0;
    rb->cap = 0;
    rb->overflowed = false;
}

void rb_clear(RenderBuffer* rb)
{
    if (!rb) return;
    rb->len = 0;
}

enum result rb_reserve(RenderBuffer* rb, size_t extra)
{
    if (!rb || extra == 0) {
        return ok;
    }
    if (extra > SIZE_MAX - rb->len) {
        rb->overflowed = true;
        return err;
    }
    if (rb_grow(rb, rb->len + extra) != ok) {
        rb->overflowed = true;
        return err;
    }
    return ok;
}

enum result rb_append(RenderBuffer* rb, const void* src, size_t size)
{
    if (!rb) {
        return err;
    }
    if (!src || size == 0) {
        return ok; /* appending nothing is not an error */
    }
    if (rb_reserve(rb, size) != ok) {
        return err;
    }

    memcpy(rb->data + rb->len, src, size);
    rb->len += size;
    return ok;
}

enum result rb_append_char(RenderBuffer* rb, char ch)
{
    if (!rb) {
        return err;
    }
    if (rb_reserve(rb, 1) != ok) {
        return err;
    }

    rb->data[rb->len++] = ch;
    return ok;
}

enum result rb_append_str(RenderBuffer* rb, const char* str)
{
    if (!str) {
        return rb ? ok : err;
    }
    return rb_append(rb, str, strlen(str));
}

void rb_flush(RenderBuffer* rb)
{
    if (!rb) return;

    size_t off = 0;
    while (off < rb->len) {
        ssize_t n = write(STDOUT_FILENO, rb->data + off, rb->len - off);
        if (n < 0) {
            if (errno == EINTR) {
                continue; /* interrupted: retry the same range */
            }
            break; /* terminal went away: drop the rest of the frame */
        }
        if (n == 0) {
            break; /* no forward progress possible; avoid spinning */
        }
        off += (size_t)n;
    }

    rb->len = 0;
}
