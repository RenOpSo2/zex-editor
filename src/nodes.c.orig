#include "nodes.h"
#include "global.h"
#include <string.h>
#include <unistd.h>

static struct page* page_new(Arena* arena) {
    struct page* p = arena_cnew(arena, struct page);
    if (!p) return NULL; // Arena exhausted — let the caller decide what to do.
    p->gap_start = 0;
    p->gap_end = PAGE_CAPACITY;
    p->next = NULL;
    p->prev = NULL;
    return p;
}

void pgb_init(struct paged_gap_buffer* pgb, Arena* arena) {
    pgb->head = page_new(arena);
    // page_new only fails if the arena has no room for a single page (~4KB);
    // with the editor's 16MB arena this should never happen at startup, but
    // fail loudly here rather than leaving head/tail/active_page as NULL and
    // crashing unpredictably later on first use.
    if (!pgb->head) {
        static char emergency_msg[] = "zex: out of memory during startup\n";
        write(STDERR_FILENO, emergency_msg, sizeof(emergency_msg) - 1);
        _exit(1);
    }
    pgb->tail = pgb->head;
    pgb->active_page = pgb->head;
}

static void page_split(struct paged_gap_buffer* pgb, Arena* arena) {
    struct page* curr = pgb->active_page;
    struct page* new_page = page_new(arena);
    if (!new_page) return; // Arena exhausted — caller (pgb_insert) will drop the edit.

    uint32_t right_len = PAGE_CAPACITY - curr->gap_end;
    new_page->gap_end = PAGE_CAPACITY - right_len;

    if (right_len > 0) {
        memcpy(new_page->data + new_page->gap_end, curr->data + curr->gap_end, right_len);
    }

    curr->gap_end = PAGE_CAPACITY;

    new_page->prev = curr;
    new_page->next = curr->next;
    if (curr->next) curr->next->prev = new_page;
    else pgb->tail = new_page;
    curr->next = new_page;

    if (curr->gap_start == PAGE_CAPACITY) {
        pgb->active_page = new_page;
    }
}

void pgb_insert(struct paged_gap_buffer* pgb, char ch, Arena* arena) {
    struct page* p = pgb->active_page;
    if (p->gap_start == p->gap_end) {
        page_split(pgb, arena);
        p = pgb->active_page;
        if (p->gap_start == p->gap_end) {
            // Split failed (arena exhausted): no room left, drop the
            // character instead of writing out of bounds or crashing.
            return;
        }
    }
    p->data[p->gap_start++] = ch;
}

void pgb_delete(struct paged_gap_buffer* pgb) {
    struct page* p = pgb->active_page;
    if (p->gap_start > 0) {
        p->gap_start--;
    } else if (p->prev) {
        pgb->active_page = p->prev;
        p = pgb->active_page;
        while (p->gap_end < PAGE_CAPACITY) {
            p->data[p->gap_start++] = p->data[p->gap_end++];
        }
        pgb_delete(pgb);
    }
}

void pgb_clear(struct paged_gap_buffer* pgb) {
    struct page* p = pgb->head;
    while (p) {
        p->gap_start = 0;
        p->gap_end = PAGE_CAPACITY;
        p = p->next;
    }
    pgb->active_page = pgb->head;
}

void pgb_insert_str(struct paged_gap_buffer* pgb, const char* src, Arena* arena) {
    for (uint32_t i = 0; src[i] != '\0'; i++) {
        pgb_insert(pgb, src[i], arena);
    }
}

void pgb_replace_str(struct paged_gap_buffer* pgb, const char* src, Arena* arena) {
    pgb_clear(pgb);
    pgb_insert_str(pgb, src, arena);
}

void pgb_to_str(char* dst, size_t dst_size, struct paged_gap_buffer* pgb) {
    uint32_t i = 0;
    struct page* p = pgb->head;
    while (p && i < dst_size - 1) {
        for (uint32_t j = 0; j < p->gap_start && i < dst_size - 1; j++) dst[i++] = p->data[j];
        for (uint32_t j = p->gap_end; j < PAGE_CAPACITY && i < dst_size - 1; j++) dst[i++] = p->data[j];
        p = p->next;
    }
    dst[i] = '\0';
}

void pgb_move_left(struct paged_gap_buffer* pgb) {
    struct page* p = pgb->active_page;
    if (p->gap_start > 0) {
        p->gap_end--;
        p->gap_start--;
        p->data[p->gap_end] = p->data[p->gap_start];
    } else if (p->prev) {
        pgb->active_page = p->prev;
        p = pgb->active_page;
        while (p->gap_end < PAGE_CAPACITY) {
            p->data[p->gap_start++] = p->data[p->gap_end++];
        }
        if (p->gap_start > 0) {
            p->gap_end--;
            p->gap_start--;
            p->data[p->gap_end] = p->data[p->gap_start];
        }
    }
}

void pgb_move_right(struct paged_gap_buffer* pgb) {
    struct page* p = pgb->active_page;
    if (p->gap_end < PAGE_CAPACITY) {
        p->data[p->gap_start++] = p->data[p->gap_end++];
    } else if (p->next) {
        pgb->active_page = p->next;
        p = pgb->active_page;
        while (p->gap_start > 0) {
            p->gap_end--;
            p->gap_start--;
            p->data[p->gap_end] = p->data[p->gap_start];
        }
        if (p->gap_end < PAGE_CAPACITY) {
            p->data[p->gap_start++] = p->data[p->gap_end++];
        }
    }
}

void pgb_move_up(struct paged_gap_buffer* pgb) {
    uint32_t col = 0;
    while (1) {
        struct page* p = pgb->active_page;
        if (p->gap_start == 0 && !p->prev) break;
        pgb_move_left(pgb);
        p = pgb->active_page;
        if (p->data[p->gap_start] == '\n') {
            pgb_move_right(pgb);
            break;
        }
        col++;
    }
    if (pgb->active_page->gap_start == 0 && !pgb->active_page->prev) {
        while (col > 0) {
            pgb_move_right(pgb);
            col--;
        }
        return;
    }
    pgb_move_left(pgb);
    uint32_t prev_line_len = 0;
    while (1) {
        struct page* p = pgb->active_page;
        if (p->gap_start == 0 && !p->prev) break;
        pgb_move_left(pgb);
        p = pgb->active_page;
        if (p->data[p->gap_start] == '\n') {
            pgb_move_right(pgb);
            break;
        }
        prev_line_len++;
    }
    uint32_t target = col < prev_line_len ? col : prev_line_len;
    for (uint32_t i = 0; i < target; i++) {
        pgb_move_right(pgb);
    }
}

void pgb_move_down(struct paged_gap_buffer* pgb) {
    uint32_t col = 0;
    while (1) {
        struct page* p = pgb->active_page;
        if (p->gap_start == 0 && !p->prev) break;
        pgb_move_left(pgb);
        p = pgb->active_page;
        if (p->data[p->gap_start] == '\n') {
            pgb_move_right(pgb);
            break;
        }
        col++;
    }
    for (uint32_t i = 0; i < col; i++) {
        pgb_move_right(pgb);
    }
    while (1) {
        struct page* p = pgb->active_page;
        if (p->gap_end == PAGE_CAPACITY && !p->next) return;
        pgb_move_right(pgb);
        p = pgb->active_page;
        if (p->data[p->gap_start - 1] == '\n') {
            break;
        }
    }
    for (uint32_t i = 0; i < col; i++) {
        struct page* p = pgb->active_page;
        if (p->gap_end == PAGE_CAPACITY && !p->next) break;
        pgb_move_right(pgb);
        p = pgb->active_page;
        if (p->data[p->gap_start - 1] == '\n') {
            pgb_move_left(pgb);
            break;
        }
    }
}

// --- Selection & clipboard helpers ---

// Returns the cursor's current linear byte offset from the start of the buffer.
uint32_t pgb_cursor_pos(struct paged_gap_buffer* pgb) {
    uint32_t pos = 0;
    struct page* p = pgb->head;
    while (p) {
        if (p == pgb->active_page) {
            pos += p->gap_start;
            return pos;
        }
        pos += p->gap_start + (PAGE_CAPACITY - p->gap_end);
        p = p->next;
    }
    return pos;
}

// Move cursor to a specific linear byte offset.
void pgb_move_to_pos(struct paged_gap_buffer* pgb, uint32_t target) {
    // Move to start first.
    while (pgb->active_page->prev) {
        pgb->active_page = pgb->active_page->prev;
    }
    while (pgb->active_page->gap_start > 0) {
        pgb->active_page->gap_end--;
        pgb->active_page->gap_start--;
        pgb->active_page->data[pgb->active_page->gap_end] =
            pgb->active_page->data[pgb->active_page->gap_start];
    }
    // Advance right by target steps.
    for (uint32_t i = 0; i < target; i++) {
        struct page* p = pgb->active_page;
        if (p->gap_end == PAGE_CAPACITY && !p->next) break;
        pgb_move_right(pgb);
    }
}

// Copy logical bytes [from, to) from src into dst clipboard.
void pgb_copy_range(struct paged_gap_buffer* dst, struct paged_gap_buffer* src,
                    uint32_t from, uint32_t to, Arena* arena) {
    pgb_clear(dst);
    if (from >= to) return;

    uint32_t pos = 0;
    struct page* p = src->head;

    while (p && pos < to) {
        // Before-gap section of this page
        for (uint32_t i = 0; i < p->gap_start && pos < to; i++, pos++) {
            if (pos >= from) pgb_insert(dst, p->data[i], arena);
        }
        // After-gap section
        for (uint32_t i = p->gap_end; i < PAGE_CAPACITY && pos < to; i++, pos++) {
            if (pos >= from) pgb_insert(dst, p->data[i], arena);
        }
        p = p->next;
    }
}

// Delete logical bytes [from, to) from pgb.
void pgb_delete_range(struct paged_gap_buffer* pgb, uint32_t from, uint32_t to) {
    if (from >= to) return;
    pgb_move_to_pos(pgb, to);
    uint32_t count = to - from;
    for (uint32_t i = 0; i < count; i++) {
        pgb_delete(pgb);
    }
}

// --- Undo/redo functions ---

void undo_save_insert(struct global* global, char ch, uint32_t pos) {
    if (global->undo_count >= UNDO_STACK_SIZE) return;
    
    struct action* act = &global->undo_stack[global->undo_count];
    act->type = action_insert;
    act->data[0] = ch;
    act->len = 1;
    act->pos = pos;
    global->undo_count++;
    
    // Clear redo stack on new action
    global->redo_count = 0;
}

void undo_save_delete(struct global* global, char ch, uint32_t pos) {
    if (global->undo_count >= UNDO_STACK_SIZE) return;
    
    struct action* act = &global->undo_stack[global->undo_count];
    act->type = action_delete;
    act->data[0] = ch;
    act->len = 1;
    act->pos = pos;
    global->undo_count++;
    
    // Clear redo stack on new action
    global->redo_count = 0;
}

void undo_perform(struct global* global) {
    if (global->undo_count == 0) return;
    
    struct action* act = &global->undo_stack[global->undo_count - 1];
    
    // Save to redo stack
    if (global->redo_count < UNDO_STACK_SIZE) {
        global->redo_stack[global->redo_count] = *act;
        global->redo_count++;
    }
    
    // Perform undo
    if (act->type == action_insert) {
        // Undo insert = delete the character that was inserted
        // Move to position AFTER the insertion (cursor is now after the char)
        pgb_move_to_pos(&global->text, act->pos + 1);
        for (uint32_t i = 0; i < act->len; i++) {
            pgb_delete(&global->text);
        }
    } else if (act->type == action_delete) {
        // Undo delete = insert the character that was deleted
        // Move to position where it was deleted, then insert
        pgb_move_to_pos(&global->text, act->pos);
        for (uint32_t i = 0; i < act->len; i++) {
            pgb_insert(&global->text, act->data[i], &global->arena);
        }
    }
    
    global->undo_count--;
}

void redo_perform(struct global* global) {
    if (global->redo_count == 0) return;
    
    struct action* act = &global->redo_stack[global->redo_count - 1];
    
    // Perform redo
    if (act->type == action_insert) {
        // Redo insert = insert the character back at original position
        pgb_move_to_pos(&global->text, act->pos);
        for (uint32_t i = 0; i < act->len; i++) {
            pgb_insert(&global->text, act->data[i], &global->arena);
        }
    } else if (act->type == action_delete) {
        // Redo delete = delete the character again
        // Move to position after the character, then delete
        pgb_move_to_pos(&global->text, act->pos + 1);
        for (uint32_t i = 0; i < act->len; i++) {
            pgb_delete(&global->text);
        }
    }
    
    // Move action back to undo stack
    global->redo_count--;
    global->undo_count++;
}

// --- Search functions ---

void search_init(struct global* global) {
    global->search_active = false;
    global->search_query[0] = '\0';
    global->search_pos = 0;
    global->search_match_count = 0;
}

// Find all occurrences of query in text, return first match position
void search_find(struct global* global, const char* query) {
    if (query[0] == '\0') {
        global->search_active = false;
        global->search_match_count = 0;
        return;
    }
    
    strncpy(global->search_query, query, sizeof(global->search_query) - 1);
    global->search_query[sizeof(global->search_query) - 1] = '\0';
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global->text);
    
    uint32_t count = 0;
    uint32_t first_match = (uint32_t)-1;
    
    char* ptr = buffer;
    while ((ptr = strstr(ptr, query)) != NULL) {
        uint32_t pos = ptr - buffer;
        if (count == 0) {
            first_match = pos;
        }
        count++;
        ptr++; // Move past this match
    }
    
    global->search_match_count = count;
    global->search_active = (count > 0);
    global->search_pos = first_match;
    
    if (first_match != (uint32_t)-1) {
        pgb_move_to_pos(&global->text, first_match);
    }
}

// Move to next search match
void search_next(struct global* global) {
    if (!global->search_active || global->search_query[0] == '\0') return;
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global->text);
    
    char* query = global->search_query;
    uint32_t query_len = strlen(query);
    uint32_t total_size = strlen(buffer);
    
    if (global->search_pos == (uint32_t)-1) {
        char* ptr = strstr(buffer, query);
        if (ptr) {
            global->search_pos = ptr - buffer;
            pgb_move_to_pos(&global->text, global->search_pos);
        }
        return;
    }
    
    uint32_t start_pos = global->search_pos + query_len;
    char* ptr = NULL;
    if (start_pos < total_size) {
        ptr = strstr(buffer + start_pos, query);
    }
    
    if (ptr) {
        global->search_pos = ptr - buffer;
        pgb_move_to_pos(&global->text, global->search_pos);
    } else {
        ptr = strstr(buffer, query);
        if (ptr) {
            global->search_pos = ptr - buffer;
            pgb_move_to_pos(&global->text, global->search_pos);
        }
    }
}

// Move to previous search match
void search_prev(struct global* global) {
    if (!global->search_active || global->search_query[0] == '\0') return;
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global->text);
    
    char* query = global->search_query;
    uint32_t current_pos = global->search_pos;
    uint32_t best_match = (uint32_t)-1;
    uint32_t last_match = (uint32_t)-1;
    
    char* ptr = buffer;
    while ((ptr = strstr(ptr, query)) != NULL) {
        uint32_t pos = ptr - buffer;
        last_match = pos;
        if (current_pos != (uint32_t)-1 && pos < current_pos) {
            best_match = pos;
        }
        ptr++;
    }
    
    if (current_pos == (uint32_t)-1 || best_match == (uint32_t)-1) {
        if (last_match != (uint32_t)-1) {
            global->search_pos = last_match;
            pgb_move_to_pos(&global->text, last_match);
        }
    } else {
        global->search_pos = best_match;
        pgb_move_to_pos(&global->text, best_match);
    }
}
