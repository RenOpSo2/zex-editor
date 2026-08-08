#include <stdio.h>
#include <stdlib.h>
#include "arena.h"

typedef struct Line {
    char *text;
    size_t length;
    size_t capacity;
} Line;

typedef struct TextBuffer {
    Line *lines;
    size_t num_lines;
    size_t capacity;
    Arena arena;  // All allocations come from this
} TextBuffer;

// Static buffer for the arena (adjust size as needed)
#define ARENA_SIZE (1024 * 1024)  // 1MB

int main() {
    // Allocate static memory for the arena
    static char arena_memory[ARENA_SIZE];
    Arena arena = arena_init(arena_memory, ARENA_SIZE);

    // Create text buffer using arena
    TextBuffer *buffer = arena_new(&arena, TextBuffer);
    buffer->arena = arena;

    // Allocate initial line array
    buffer->lines = arena_new_array(&arena, Line, 100);
    buffer->capacity = 100;
    buffer->num_lines = 0;

    Line *line = &buffer->lines[buffer->num_lines++];
    line->text = arena_strdup(&arena, "Hello, World!");
    line->length = 13;

    line = &buffer->lines[buffer->num_lines++];
    line->text = arena_strdup(&arena, "This is a text editor!");
    line->length = 21;

    for (size_t i = 0; i < buffer->num_lines; i++) {
        printf("Line %zu: %s\n", i, buffer->lines[i].text);
    }

    // Reset everything when needed (e.g., closing a file)
    arena_reset(&arena);

    // Or reuse arena for new buffer
    buffer = arena_new(&arena, TextBuffer);
    buffer->arena = arena;
    // ... etc

    return 0;
}
