#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>
#include <sys/ioctl.h>
#include <stddef.h>
#include "../libmemory/arena.h"

#define arena_capacity (1 << 24)
#define term_capacity (1 << 16)
#define buf_capacity (1 << 16)

#define PAGE_CAPACITY 4096
#define UNDO_STACK_SIZE 100

// Ctrl+key macros
#define CTRL_KEY(k) ((k) & 0x1f)

// Undo/redo action types
enum action_type {
    action_insert,
    action_delete,
    action_replace
};

struct action {
    enum action_type type;
    char data[256];  // Store inserted/deleted text
    uint32_t pos;    // Cursor position
    uint32_t len;    // Length of data
};

enum result {
    ok = 0,
    err = 1,
};
enum bool {
    false = 0,
    true = 1,
};

struct page {
    char data[PAGE_CAPACITY];
    uint32_t gap_start;
    uint32_t gap_end;
    struct page* next;
    struct page* prev;
};

struct paged_gap_buffer {
    struct page* head;
    struct page* tail;
    struct page* active_page;
};

struct term {
    struct winsize ws;
};

struct global {
    struct term term;
    struct paged_gap_buffer text;
    struct paged_gap_buffer clipboard;
    struct paged_gap_buffer msg;
    char filepath[256];

    // Selection: linear byte offsets into the logical content.
    // sel_anchor is fixed when selection starts; cursor end is computed live.
    enum bool has_selection;
    uint32_t sel_anchor; // byte offset where selection started

    // Undo/redo stacks
    struct action undo_stack[UNDO_STACK_SIZE];
    struct action redo_stack[UNDO_STACK_SIZE];
    uint32_t undo_count;
    uint32_t redo_count;

    // Search state
    enum bool search_active;
    char search_query[256];
    uint32_t search_pos; // current match position
    uint32_t search_match_count; // number of matches found

    Arena arena;
};

#endif
