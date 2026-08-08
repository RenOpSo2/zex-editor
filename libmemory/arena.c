#include "arena.h"
#include <string.h>

#define ARENA_DEFAULT_ALIGNMENT 16

/**
 * @brief Align pointer up to nearest multiple of alignment
 * @param ptr Raw pointer value
 * @param alignment Must be power of 2
 * @return Aligned pointer value
 */
static inline uintptr_t align_up(uintptr_t ptr, size_t alignment) {
    uintptr_t mask = (uintptr_t)alignment - 1;
    return (ptr + mask) & ~mask;
}

Arena arena_init(void *buffer, size_t size) {
    return (Arena) {
        .buf = (char*)buffer,
        .offset = 0,
        .capacity = size
    };
}

void *arena_alloc(Arena *arena, size_t size, size_t alignment) {
    // Zero-size allocations return NULL
    if (size == 0) return NULL;

    // Calculate aligned pointer from current offset
    uintptr_t curr = (uintptr_t)arena->buf + (uintptr_t)arena->offset;
    uintptr_t aligned = align_up(curr, alignment);

    // Calculate total space needed (size + alignment padding)
    size_t padding = (size_t)(aligned - curr);
    size_t total_size = size + padding;

    // Out of memory check
    if (arena->offset + total_size > arena->capacity) {
        return NULL;
    }

    // Bump the offset and return aligned pointer
    arena->offset += total_size;
    return (void*)aligned;
}

void *arena_calloc(Arena *arena, size_t count, size_t size, size_t alignment) {
    size_t total = count * size;
    void *ptr = arena_alloc(arena, total, alignment);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void arena_reset(Arena *arena) {
    arena->offset = 0;  // Instant free - O(1)
}

void *arena_alloc_default(Arena *arena, size_t size) {
    return arena_alloc(arena, size, ARENA_DEFAULT_ALIGNMENT);
}

char *arena_strdup(Arena *arena, const char *str) {
    size_t len = strlen(str);
    return arena_strndup(arena, str, len);
}

char *arena_strndup(Arena *arena, const char *str, size_t len) {
    // +1 for null terminator, alignment=1 since chars don't need alignment
    char *dup = (char*)arena_alloc(arena, len + 1, 1);
    if (dup) {
        memcpy(dup, str, len);
        dup[len] = '\0';
    }
    return dup;
}
