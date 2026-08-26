#include "render_buffer.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

namespace {

constexpr size_t kInitialCapacity = 4096u;
constexpr size_t kMaxCapacity = 64u * 1024u * 1024u;

static int grow(RenderBuffer* rb, size_t needed)
{
    if (!rb) return 1;
    if (needed <= rb->cap) return 0;

    size_t new_cap = rb->cap ? rb->cap : kInitialCapacity;
    while (new_cap < needed && new_cap <= kMaxCapacity / 2u) {
        new_cap *= 2u;
    }
    if (new_cap < needed) {
        new_cap = needed;
    }
    if (new_cap > kMaxCapacity) {
        return 1;
    }

    char* grown = static_cast<char*>(std::realloc(rb->data, new_cap));
    if (!grown) {
        return 1;
    }

    rb->data = grown;
    rb->cap = new_cap;
    return 0;
}

} // namespace

extern "C" {

void rb_init(RenderBuffer* rb)
{
    if (!rb) return;
    rb->data = nullptr;
    rb->len = 0;
    rb->cap = 0;
    rb->overflowed = 0;
}

void rb_deinit(RenderBuffer* rb)
{
    if (!rb) return;
    std::free(rb->data);
    rb->data = nullptr;
    rb->len = 0;
    rb->cap = 0;
    rb->overflowed = 0;
}

void rb_clear(RenderBuffer* rb)
{
    if (!rb) return;
    rb->len = 0;
}

int rb_reserve(RenderBuffer* rb, size_t extra)
{
    if (!rb || extra == 0) {
        return 0;
    }
    if (extra > kMaxCapacity || rb->len > kMaxCapacity - extra) {
        rb->overflowed = 1;
        return 1;
    }
    if (grow(rb, rb->len + extra) != 0) {
        rb->overflowed = 1;
        return 1;
    }
    return 0;
}

int rb_append(RenderBuffer* rb, const void* src, size_t size)
{
    if (!rb) {
        return 1;
    }
    if (!src || size == 0) {
        return 0;
    }
    if (rb_reserve(rb, size) != 0) {
        return 1;
    }

    std::memcpy(rb->data + rb->len, src, size);
    rb->len += size;
    return 0;
}

int rb_append_char(RenderBuffer* rb, char ch)
{
    if (!rb) {
        return 1;
    }
    if (rb_reserve(rb, 1) != 0) {
        return 1;
    }

    rb->data[rb->len++] = ch;
    return 0;
}

int rb_append_str(RenderBuffer* rb, const char* str)
{
    if (!str) {
        return rb ? 0 : 1;
    }
    return rb_append(rb, str, std::strlen(str));
}

void rb_flush(RenderBuffer* rb)
{
    if (!rb) return;

    size_t off = 0;
    while (off < rb->len) {
        ssize_t n = ::write(STDOUT_FILENO, rb->data + off, rb->len - off);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (n == 0) {
            break;
        }
        off += static_cast<size_t>(n);
    }

    rb->len = 0;
}

} // extern "C"