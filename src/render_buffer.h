#ifndef RENDER_BUFFER_H
#define RENDER_BUFFER_H

#include "global.h"
#include <stddef.h>

/**
 * RenderBuffer - growable, bounds-checked byte buffer used to assemble a
 * complete terminal frame before it is written to stdout.
 *
 * Safety contract:
 *  - Appends never write out of bounds and never truncate silently: if an
 *    append cannot be satisfied (out of memory), the request is dropped and
 *    rb->overflowed is set so the caller can detect the lost frame.
 *  - Storage is allocated lazily and only grows; after warm-up a steady-state
 *    frame performs no allocation at all.
 *  - Every function tolerates NULL inputs unless documented otherwise.
 *  - rb_flush() copes with partial writes and EINTR instead of corrupting
 *    the frame like a single unchecked write() would.
 */
typedef struct {
    char* data;
    size_t len;
    size_t cap;
    enum bool overflowed;
} RenderBuffer;

/**
 * Reset a buffer to empty without releasing storage.
 * Safe to call on a zeroed buffer (storage is allocated lazily).
 */
void rb_init(RenderBuffer* rb);

/** Release all storage owned by the buffer. */
void rb_deinit(RenderBuffer* rb);

/** Drop the contents but keep the allocated storage. */
void rb_clear(RenderBuffer* rb);

/**
 * Ensure room for at least `extra` more bytes.
 * Returns ok, or err if the allocation failed (overflowed is set).
 */
enum result rb_reserve(RenderBuffer* rb, size_t extra);

/** Append `size` bytes from src. Dropping an empty append is not an error. */
enum result rb_append(RenderBuffer* rb, const void* src, size_t size);
enum result rb_append_char(RenderBuffer* rb, char ch);
enum result rb_append_str(RenderBuffer* rb, const char* str);

/**
 * Write the buffered bytes to stdout, retrying on partial writes and
 * EINTR. Always leaves the buffer empty. Errors are swallowed because a
 * dead terminal cannot be reported to anyway.
 */
void rb_flush(RenderBuffer* rb);

#endif
