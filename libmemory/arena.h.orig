#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct Arena
 * @brief Simple bump allocator for fast, leak-free memory management
 *
 * All memory is freed at once with arena_reset(). Perfect for:
 * - Text editor buffers (one arena per file)
 * - Parsers & compilers (AST nodes, tokens)
 * - Game engines (per-frame allocations)
 * - Request handlers (scoped HTTP request memory)
 */
typedef struct Arena {
    char *buf;        /**< Underlying memory buffer */
    size_t offset;    /**< Current allocation position */
    size_t capacity;  /**< Total buffer size */
} Arena;

/**
 * @brief Initialize arena with pre-allocated buffer
 * @param buffer Pointer to static/stack/heap memory block
 * @param size Size of buffer in bytes
 * @return Initialized Arena (zero overhead beyond the 3 fields)
 *
 * Example:
 *   static char mem[1024*1024];  // 1MB
 *   Arena arena = arena_init(mem, sizeof(mem));
 */
Arena arena_init(void *buffer, size_t size);

/**
 * @brief Allocate aligned memory from arena
 * @param arena Pointer to arena
 * @param size Bytes to allocate
 * @param alignment Must be power of 2 (e.g., 1, 2, 4, 8, 16, 32)
 * @return Pointer to aligned memory, or NULL if out of space
 *
 * Performance: O(1) - just bumps offset
 * Overhead: 0-15 bytes padding per allocation for alignment
 */
void *arena_alloc(Arena *arena, size_t size, size_t alignment);

/**
 * @brief Allocate and zero-initialize memory
 * @param arena Pointer to arena
 * @param count Number of elements
 * @param size Size of each element
 * @param alignment Memory alignment
 * @return Zeroed pointer or NULL
 */
void *arena_calloc(Arena *arena, size_t count, size_t size, size_t alignment);

/**
 * @brief Reset arena to empty state (O(1))
 * @param arena Pointer to arena
 *
 * Instantly frees all allocations. No individual free() needed.
 * The buffer memory remains valid for reuse.
 *
 * Typical text editor usage:
 *   arena_reset(&buffer->arena);  // Close file
 *   // ... arena ready for next file
 */
void arena_reset(Arena *arena);

/**
 * @brief Duplicate a null-terminated string
 * @param arena Arena to allocate from
 * @param str Source string
 * @return Newly allocated copy, or NULL if out of memory
 *
 * Allocates strlen(str) + 1 bytes (includes null terminator)
 */
char *arena_strdup(Arena *arena, const char *str);

/**
 * @brief Duplicate string with explicit length
 * @param arena Arena to allocate from
 * @param str Source string (may not be null-terminated)
 * @param len Number of characters to copy
 * @return Null-terminated copy, or NULL
 *
 * Useful for:
 *   char *word = arena_strndup(arena, line + start, end - start);
 */
char *arena_strndup(Arena *arena, const char *str, size_t len);

/**
 * @brief Allocate with default 16-byte alignment
 * @param arena Arena to allocate from
 * @param size Bytes needed
 * @return Aligned pointer or NULL
 *
 * Good default for general purpose allocations
 */
void *arena_alloc_default(Arena *arena, size_t size);

// ============ CONVENIENCE MACROS ============

/**
 * @brief Allocate and cast a single object
 * @code
 *   Line *line = arena_new(&arena, Line);
 *   Buffer *buf = arena_new(&arena, Buffer);
 * @endcode
 */
#define arena_new(arena, type) \
    ((type*)arena_alloc((arena), sizeof(type), alignof(type)))

/**
 * @brief Allocate array of objects
 * @code
 *   Line *lines = arena_new_array(&arena, Line, 100);
 *   Token *tokens = arena_new_array(&arena, Token, count);
 * @endcode
 */
#define arena_new_array(arena, type, count) \
    ((type*)arena_alloc((arena), sizeof(type) * (count), alignof(type)))

/**
 * @brief Allocate and zero a single object
 * @code
 *   Line *line = arena_cnew(&arena, Line);
 * @endcode
 */
#define arena_cnew(arena, type) \
    ((type*)arena_calloc((arena), 1, sizeof(type), alignof(type)))

/**
 * @brief Allocate and zero array of objects
 * @code
 *   int *array = arena_cnew_array(&arena, int, 1000);
 * @endcode
 */
#define arena_cnew_array(arena, type, count) \
    ((type*)arena_calloc((arena), (count), sizeof(type), alignof(type)))

#ifdef __cplusplus
}
#endif

#endif
