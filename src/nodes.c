/**
 * nodes.c - Paged Gap Buffer Implementation
 * 
 * Implements a gap buffer split across multiple fixed-size pages for efficient
 * text editing operations. The gap buffer allows O(1) insert/delete at cursor
 * position with minimal memory movement.
 * 
 * Key concepts:
 * - Each page has a gap (unused space) where edits happen
 * - Pages are linked bidirectionally for navigation
 * - Cursor position is maintained via gap_start/gap_end pointers
 * - Supports undo/redo, search, selection, and clipboard operations
 */

#include "nodes.h"
#include "global.h"
#include "config.h"
#include "dispwidth.h"
#include <string.h>
#include <unistd.h>
#include <stddef.h>

/**
 * page_new - Allocate and initialize a new page
 * @arena: Memory arena for allocation
 * 
 * Returns: New page with full gap, or NULL if allocation fails
 * 
 * A new page starts with 100% gap (gap_start=0, gap_end=PAGE_CAPACITY)
 * meaning no actual content yet.
 */
static struct page* page_new(Arena* arena)
{
    struct page* p = arena_cnew(arena, struct page);
    if (!p) return NULL; // Arena exhausted — let the caller decide what to do.
    p->gap_start = 0;
    p->gap_end = PAGE_CAPACITY;
    p->next = NULL;
    p->prev = NULL;
    return p;
}

/**
 * pgb_init - Initialize the paged gap buffer
 * @pgb: Pointer to paged gap buffer structure
 * @arena: Memory arena for page allocations
 * 
 * Creates the initial page and sets up the buffer structure.
 * Exits with error if initial allocation fails (critical failure).
 */
void pgb_init(struct paged_gap_buffer* pgb, Arena* arena)
{
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

/**
 * page_split - Split current page when gap is full
 * @pgb: Paged gap buffer
 * @arena: Memory arena for new page allocation
 * 
 * When a page's gap is exhausted (gap_start == gap_end), creates a new page
 * and moves half the content after the gap to the new page.
 * 
 * This maintains O(1) insertion by ensuring there's always gap space.
 * If allocation fails, the edit is dropped (silent failure).
 */
static void page_split(struct paged_gap_buffer* pgb, Arena* arena)
{
    struct page* curr = pgb->active_page;
    struct page* new_page = page_new(arena);
    if (!new_page) return; // Arena exhausted — caller (pgb_insert) will drop the edit.

    // Calculate how much content is after the gap
    uint32_t right_len = PAGE_CAPACITY - curr->gap_end;
    new_page->gap_end = PAGE_CAPACITY - right_len;

    // Move content after gap to new page
    if (right_len > 0) {
        memcpy(new_page->data + new_page->gap_end, curr->data + curr->gap_end, right_len);
    }

    // Current page now has no content after gap
    curr->gap_end = PAGE_CAPACITY;

    // Insert new page after current page
    new_page->prev = curr;
    new_page->next = curr->next;
    if (curr->next) curr->next->prev = new_page;
    else pgb->tail = new_page;
    curr->next = new_page;

    // If current page was completely filled, move to new page
    if (curr->gap_start == PAGE_CAPACITY) {
        pgb->active_page = new_page;
    }
}

/**
 * compact_page_after_gap - Move all content after gap to before gap
 * @p: Page to compact
 * 
 * Helper function that compacts a page by moving content from after
 * the gap to before the gap. Used when moving cursor between pages.
 */
static void compact_page_after_gap(struct page* p)
{
    if (!p) return;
    while (p->gap_end < PAGE_CAPACITY) {
        p->data[p->gap_start++] = p->data[p->gap_end++];
    }
}

/**
 * compact_page_before_gap - Move all content before gap to after gap
 * @p: Page to compact
 * 
 * Helper function that compacts a page by moving content from before
 * the gap to after the gap. Used when moving cursor between pages.
 */
static void compact_page_before_gap(struct page* p)
{
    if (!p) return;
    while (p->gap_start > 0) {
        p->gap_end--;
        p->gap_start--;
        p->data[p->gap_end] = p->data[p->gap_start];
    }
}

/**
 * pgb_insert - Insert a single character at cursor position
 * @pgb: Paged gap buffer
 * @ch: Character to insert
 * @arena: Memory arena for page allocation
 * 
 * Inserts character at current cursor position (gap_start).
 * If gap is full, splits page to create more space.
 * If split fails, character is silently dropped.
 */
void pgb_insert(struct paged_gap_buffer* pgb, char ch, Arena* arena)
{
    if (!pgb || !pgb->active_page) return;
    struct page* p = pgb->active_page;
    if (p->gap_start == p->gap_end) {
        page_split(pgb, arena);
        p = pgb->active_page;
        if (!p || p->gap_start == p->gap_end) {
            // Split failed (arena exhausted): no room left, drop the
            // character instead of writing out of bounds or crashing.
            return;
        }
    }
    p->data[p->gap_start++] = ch;
}

/**
 * pgb_delete - Delete character before cursor (backspace)
 * @pgb: Paged gap buffer
 * 
 * Deletes the character immediately before the cursor.
 * If cursor is at start of a page but not at buffer start,
 * merges with previous page by pulling content.
 */
void pgb_delete(struct paged_gap_buffer* pgb)
{
    if (!pgb || !pgb->active_page) return;
    struct page* p = pgb->active_page;
    if (p->gap_start > 0) {
        p->gap_start--;  // Simple case: gap expands left
    } else if (p->prev) {
        // Cursor at start of page - need to pull content from previous page
        pgb->active_page = p->prev;
        p = pgb->active_page;
        if (!p) return;
        // Move all content after gap to before gap (compact page)
        compact_page_after_gap(p);
        // Recursively delete (now works on previous page's content)
        pgb_delete(pgb);
    }
}

/**
 * pgb_clear - Clear entire buffer (reset all pages)
 * @pgb: Paged gap buffer
 * 
 * Resets all pages to empty state (full gap).
 * Cursor moves to first page start.
 */
void pgb_clear(struct paged_gap_buffer* pgb)
{
    if (!pgb || !pgb->head) return;
    struct page* p = pgb->head;
    while (p) {
        p->gap_start = 0;
        p->gap_end = PAGE_CAPACITY;
        p = p->next;
    }
    pgb->active_page = pgb->head;
}

/**
 * pgb_insert_str - Insert a string at cursor position
 * @pgb: Paged gap buffer
 * @src: Null-terminated string to insert
 * @arena: Memory arena for page allocation
 * 
 * Convenience wrapper that inserts characters one by one.
 */
void pgb_insert_str(struct paged_gap_buffer* pgb, const char* src, Arena* arena)
{
    for (uint32_t i = 0; src[i] != '\0'; i++) {
        pgb_insert(pgb, src[i], arena);
    }
}

/**
 * pgb_replace_str - Replace entire buffer content with a string
 * @pgb: Paged gap buffer
 * @src: Null-terminated string to replace with
 * @arena: Memory arena for page allocation
 * 
 * Clears buffer then inserts the new string.
 */
void pgb_replace_str(struct paged_gap_buffer* pgb, const char* src, Arena* arena)
{
    pgb_clear(pgb);
    pgb_insert_str(pgb, src, arena);
}

/**
 * pgb_to_str - Flatten buffer to a single string
 * @dst: Destination buffer
 * @dst_size: Size of destination buffer (including null terminator)
 * @pgb: Source paged gap buffer
 *
 * Copies all logical content into a flat string.
 * Truncates if dst_size is insufficient.
 * Always null-terminates the result.
 */
void pgb_to_str(char* dst, size_t dst_size, const struct paged_gap_buffer* pgb)
{
    /* Guard against NULL outputs and the dst_size == 0 underflow. */
    if (!dst || dst_size == 0) return;
    if (!pgb) {
        dst[0] = '\0';
        return;
    }

    uint32_t i = 0;
    struct page* p = pgb->head;
    while (p && i < dst_size - 1) {
        // Copy content before gap
        for (uint32_t j = 0; j < p->gap_start && i < dst_size - 1; j++) dst[i++] = p->data[j];
        // Copy content after gap
        for (uint32_t j = p->gap_end; j < PAGE_CAPACITY && i < dst_size - 1; j++) dst[i++] = p->data[j];
        p = p->next;
    }
    dst[i] = '\0';
}

// ========== Streaming Reader ==========

/**
 * pgb_reader_init - Position a reader at the first logical byte
 * @it: Reader to initialise
 * @pgb: Buffer to read (may be NULL, yielding an empty stream)
 */
void pgb_reader_init(struct pgb_reader* it, const struct paged_gap_buffer* pgb)
{
    if (!it) return;
    it->page = pgb ? pgb->head : NULL;
    it->idx = 0;
    it->phase = 0;
}

/**
 * pgb_reader_next - Advance the reader by one logical byte
 * @it: Reader initialised with pgb_reader_init
 *
 * Returns: Next stored byte (0..255), or -1 once every page is drained.
 * Tolerates out-of-range gap indices defensively instead of reading
 * past a page's storage.
 */
int pgb_reader_next(struct pgb_reader* it)
{
    if (!it) return -1;

    for (;;) {
        const struct page* p = it->page;
        if (!p || it->phase >= 2) {
            it->phase = 2;
            return -1;
        }

        if (it->phase == 0) {
            /* Segment before the gap. */
            if (it->idx < p->gap_start && it->idx < PAGE_CAPACITY) {
                return (unsigned char)p->data[it->idx++];
            }
            it->phase = 1;
            it->idx = (p->gap_end <= PAGE_CAPACITY) ? p->gap_end : PAGE_CAPACITY;
        } else {
            /* Segment after the gap. */
            if (it->idx < PAGE_CAPACITY) {
                return (unsigned char)p->data[it->idx++];
            }
            it->page = p->next;
            it->phase = 0;
            it->idx = 0;
        }
    }
}

/**
 * pgb_move_left - Move cursor one character left
 * @pgb: Paged gap buffer
 * 
 * Moves gap left by swapping character before gap with gap position.
 * If at page start, moves to previous page's end.
 */
void pgb_move_left(struct paged_gap_buffer* pgb)
{
    if (!pgb || !pgb->active_page) return;
    struct page* p = pgb->active_page;
    if (p->gap_start > 0) {
        // Move gap left: swap character before gap into gap
        p->gap_end--;
        p->gap_start--;
        p->data[p->gap_end] = p->data[p->gap_start];
    } else if (p->prev) {
        // Move to previous page
        pgb->active_page = p->prev;
        p = pgb->active_page;
        if (!p) return;
        // Compact current page (move all content after gap to before gap)
        compact_page_after_gap(p);
        // Now move left one character on this page
        if (p->gap_start > 0) {
            p->gap_end--;
            p->gap_start--;
            p->data[p->gap_end] = p->data[p->gap_start];
        }
    }
}

/**
 * pgb_move_right - Move cursor one character right
 * @pgb: Paged gap buffer
 * 
 * Moves gap right by swapping character after gap with gap position.
 * If at page end, moves to next page's start.
 */
void pgb_move_right(struct paged_gap_buffer* pgb)
{
    if (!pgb || !pgb->active_page) return;
    struct page* p = pgb->active_page;
    if (p->gap_end < PAGE_CAPACITY) {
        // Move gap right: swap character after gap into gap
        p->data[p->gap_start++] = p->data[p->gap_end++];
    } else if (p->next) {
        // Move to next page
        pgb->active_page = p->next;
        p = pgb->active_page;
        if (!p) return;
        // Compact current page (move all content before gap to after gap)
        compact_page_before_gap(p);
        // Now move right one character on this page
        if (p->gap_end < PAGE_CAPACITY) {
            p->data[p->gap_start++] = p->data[p->gap_end++];
        }
    }
}

// ========== Cursor Navigation Helpers ==========

/**
 * get_current_column - Calculate current column position
 * @pgb: Paged gap buffer
 * 
 * Returns: Column number (0-based) from start of current line
 * 
 * Moves cursor left temporarily to count characters until newline or buffer start.
 * Restores cursor position after calculation.
 */
/* --- Display-width-aware cursor primitives ------------------------------- */

/* The byte sitting immediately to the right of the cursor (-1 at EOF). */
static int peek_next_byte(const struct paged_gap_buffer* pgb)
{
    const struct page* p = pgb->active_page;
    if (p->gap_end < PAGE_CAPACITY) return (unsigned char)p->data[p->gap_end];
    if (p->next) return (unsigned char)p->next->data[0];
    return -1;
}

/* Display width of the character whose lead byte is at page `p`, index `idx`,
 * reading any continuation bytes forward from there. Uses the same rules as
 * the renderer so the column math can never drift. */
static uint32_t width_at(struct page* p, uint32_t idx, uint32_t col, uint32_t tab_size)
{
    unsigned char buf[4];
    size_t n = 0;
    if (idx < PAGE_CAPACITY) {
        buf[n++] = (unsigned char)p->data[idx];
        int extra = utf8_trail_count(buf[0]);
        for (int k = 1; k <= extra && (idx + (uint32_t)k) < PAGE_CAPACITY; k++) {
            buf[n++] = (unsigned char)p->data[idx + (uint32_t)k];
        }
    }
    if (n == 0) return 1;
    size_t consumed;
    return char_width(buf, n, col, tab_size, &consumed);
}

/* Inspect the character to the right of the cursor without moving it.
 * *len receives the byte length (so the caller can advance by exactly that
 * many single-byte moves), *width the display columns it occupies. */
static void next_char_info(const struct paged_gap_buffer* pgb, uint32_t col,
                           uint32_t tab_size, size_t* len, uint32_t* width)
{
    const struct page* p = pgb->active_page;
    unsigned char buf[4];
    size_t got = 0;

    if (p->gap_end < PAGE_CAPACITY) {
        buf[got++] = (unsigned char)p->data[p->gap_end];
        int extra = utf8_trail_count(buf[0]);
        for (int k = 1; k <= extra; k++) {
            if (p->gap_end + (uint32_t)k < PAGE_CAPACITY) buf[got++] = (unsigned char)p->data[p->gap_end + (uint32_t)k];
            else break;
        }
    } else if (p->next) {
        const struct page* np = p->next;
        buf[got++] = (unsigned char)np->data[0];
        int extra = utf8_trail_count(buf[0]);
        for (int k = 1; k <= extra; k++) buf[got++] = (unsigned char)np->data[k];
    }

    if (got == 0) { *len = 0; *width = 0; return; }
    *width = char_width(buf, got, col, tab_size, len);
}

/**
 * get_current_column - Calculate current column position (display columns)
 * @pgb: Paged gap buffer
 *
 * Returns: Column number (0-based) from start of current line.
 *
 * Moves the cursor left to count, then restores it. Multi-byte UTF-8 is
 * counted as one display unit (the continuation bytes are skipped and the
 * lead byte accounts for the whole character).
 */
static uint32_t get_current_column(struct paged_gap_buffer* pgb)
{
    uint32_t tab_size = (uint32_t)config_get_number("tabsize", 4);
    uint32_t col = 0;
    uint32_t steps = 0;
    while (1) {
        struct page* p = pgb->active_page;
        if (p->gap_start == 0 && !p->prev) break;
        pgb_move_left(pgb);
        p = pgb->active_page;
        unsigned char b = (unsigned char)p->data[p->gap_start];
        if (b == '\n') {
            pgb_move_right(pgb);
            break;
        }
        /* Continuation bytes belong to the preceding unit; let the lead
         * byte account for the full character. */
        if ((b & 0xC0) == 0x80) {
            steps++;
            continue;
        }
        col += width_at(p, p->gap_start, col, tab_size);
        steps++;
    }
    // Restore cursor position by moving right the same number of bytes.
    for (uint32_t i = 0; i < steps; i++) {
        pgb_move_right(pgb);
    }
    return col;
}

/**
 * move_to_line_start - Move cursor to start of current line
 * @pgb: Paged gap buffer
 * 
 * Moves cursor left until newline or buffer start is reached.
 */
static void move_to_line_start(struct paged_gap_buffer* pgb)
{
    while (1) {
        struct page* p = pgb->active_page;
        if (p->gap_start == 0 && !p->prev) break;
        pgb_move_left(pgb);
        p = pgb->active_page;
        if (p->data[p->gap_start] == '\n') {
            pgb_move_right(pgb);
            break;
        }
    }
}

/**
 * get_line_length - Calculate length of current line
 * @pgb: Paged gap buffer
 * 
 * Returns: Number of characters in current line (excluding newline)
 * 
 * Moves cursor right temporarily to count characters.
 * Restores cursor position after calculation.
 */
/**
 * get_line_length - Display width of the current line (to its newline).
 * @pgb: Paged gap buffer
 *
 * Returns: Display columns occupied by the current line, excluding the
 * newline. Moves the cursor right to measure, then restores it. Tab
 * expansion, control characters and UTF-8 widths are all honoured.
 */
static uint32_t get_line_length(struct paged_gap_buffer* pgb)
{
    uint32_t tab_size = (uint32_t)config_get_number("tabsize", 4);
    uint32_t col = 0;
    uint32_t steps = 0;
    for (;;) {
        int nb = peek_next_byte(pgb);
        if (nb < 0) break;
        if (nb == '\n') {
            pgb_move_right(pgb);
            steps++;
            break;
        }
        size_t len;
        uint32_t w;
        next_char_info(pgb, col, tab_size, &len, &w);
        if (len == 0) break;
        for (size_t i = 0; i < len; i++) pgb_move_right(pgb);
        col += w;
        steps += (uint32_t)len;
    }
    // Restore cursor position by moving left the same number of bytes.
    for (uint32_t i = 0; i < steps; i++) {
        pgb_move_left(pgb);
    }
    return col;
}

/**
 * Move the cursor right to `target` display columns, stopping at the line end.
 * Used by the vertical-motion helpers to preserve the horizontal position.
 */
static void move_to_column(struct paged_gap_buffer* pgb, uint32_t target, uint32_t tab_size)
{
    uint32_t cur = 0;
    while (cur < target) {
        int nb = peek_next_byte(pgb);
        if (nb < 0 || nb == '\n') break;
        size_t len;
        uint32_t w;
        next_char_info(pgb, cur, tab_size, &len, &w);
        if (len == 0) break;
        if (cur + w > target) break; /* landing here would overshoot */
        for (size_t i = 0; i < len; i++) pgb_move_right(pgb);
        cur += w;
    }
}

/**
 * pgb_move_up - Move cursor up one line
 * @pgb: Paged gap buffer
 * 
 * Preserves horizontal position (in display columns) as much as possible.
 * If current column exceeds previous line length, snaps to line end.
 */
void pgb_move_up(struct paged_gap_buffer* pgb)
{
    uint32_t tab_size = (uint32_t)config_get_number("tabsize", 4);
    uint32_t col = get_current_column(pgb);

    // If already at first line, return (cursor already restored by get_current_column)
    if (pgb->active_page->gap_start == 0 && !pgb->active_page->prev) {
        return;
    }

    // Move to previous line: start of current line, across the newline, then
    // to the start of the previous line so get_line_length measures it fully.
    move_to_line_start(pgb);
    pgb_move_left(pgb);
    move_to_line_start(pgb);

    // Get previous line length
    uint32_t prev_line_len = get_line_length(pgb);

    // Move to target column (snap to end if needed)
    uint32_t target = col < prev_line_len ? col : prev_line_len;
    move_to_column(pgb, target, tab_size);
}

/**
 * pgb_move_down - Move cursor down one line
 * @pgb: Paged gap buffer
 * 
 * Preserves horizontal position (in display columns) as much as possible.
 * If at last line, does nothing.
 */
void pgb_move_down(struct paged_gap_buffer* pgb)
{
    uint32_t tab_size = (uint32_t)config_get_number("tabsize", 4);
    uint32_t col = get_current_column(pgb);

    // Move to next line
    for (;;) {
        int nb = peek_next_byte(pgb);
        if (nb < 0) return;               // At end of buffer
        if (nb == '\n') {
            pgb_move_right(pgb);
            break;
        }
        pgb_move_right(pgb);
    }

    // Get next line length and move to target column
    uint32_t next_line_len = get_line_length(pgb);
    uint32_t target = col < next_line_len ? col : next_line_len;
    move_to_column(pgb, target, tab_size);
}

// ========== Selection & Clipboard Operations ==========

/**
 * pgb_cursor_pos - Get current cursor position as byte offset
 * @pgb: Paged gap buffer
 * 
 * Returns: Linear byte offset from start of buffer
 * 
 * Walks through all pages to calculate absolute position.
 */
uint32_t pgb_cursor_pos(const struct paged_gap_buffer* pgb)
{
    if (!pgb || !pgb->head) return 0;
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

/**
 * pgb_move_to_pos - Move cursor to specific byte offset
 * @pgb: Paged gap buffer
 * @target: Target byte offset from start
 * 
 * Navigates to the specified position in the buffer.
 * If target is out of range, moves as far as possible.
 */
void pgb_move_to_pos(struct paged_gap_buffer* pgb, uint32_t target)
{
    if (!pgb || !pgb->active_page) return;
    
    // Move to start first
    while (pgb->active_page->prev) {
        pgb->active_page = pgb->active_page->prev;
    }
    // Compact first page completely
    compact_page_before_gap(pgb->active_page);
    // Advance right by target steps
    for (uint32_t i = 0; i < target; i++) {
        struct page* p = pgb->active_page;
        if (!p || (p->gap_end == PAGE_CAPACITY && !p->next)) break;
        pgb_move_right(pgb);
    }
}

/**
 * pgb_copy_range - Copy range of bytes to clipboard
 * @dst: Destination clipboard buffer
 * @src: Source buffer
 * @from: Start byte offset (inclusive)
 * @to: End byte offset (exclusive)
 * @arena: Memory arena for clipboard allocations
 * 
 * Copies a range of logical bytes from source to destination.
 * Clears destination before copying.
 */
void pgb_copy_range(struct paged_gap_buffer* dst, const struct paged_gap_buffer* src,
                    uint32_t from, uint32_t to, Arena* arena)
{
    if (!dst || !src) return;
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

/**
 * pgb_delete_range - Delete range of bytes
 * @pgb: Buffer to delete from
 * @from: Start byte offset (inclusive)
 * @to: End byte offset (exclusive)
 * 
 * Moves cursor to end of range and deletes backwards.
 */
void pgb_delete_range(struct paged_gap_buffer* pgb, uint32_t from, uint32_t to)
{
    if (!pgb || from >= to) return;
    pgb_move_to_pos(pgb, to);
    uint32_t count = to - from;
    for (uint32_t i = 0; i < count; i++) {
        pgb_delete(pgb);
    }
}

// ========== Undo/Redo System ==========

/**
 * undo_save_action - Save an action to the undo stack
 * @global: Global state containing undo/redo stacks
 * @type: Action type (insert, delete, or replace)
 * @data: Data associated with action
 * @len: Length of data
 * @pos: Position where action occurred
 *
 * Internal helper for undo/redo tracking.
 */
static void undo_save_action(struct global* global, enum action_type type, const char* data, uint32_t len, uint32_t pos)
{
    if (!global || !data || global->undo_count >= UNDO_STACK_SIZE) return;

    struct action* act = &global->undo_stack[global->undo_count];
    act->type = type;
    act->len = len;
    act->pos = pos;
    act->old_len = 0; // Initialize old_len for non-replace actions

    // Copy data (up to MAX_SEARCH_QUERY_LEN)
    uint32_t copy_len = len < MAX_SEARCH_QUERY_LEN ? len : MAX_SEARCH_QUERY_LEN;
    for (uint32_t i = 0; i < copy_len; i++) {
        act->data[i] = data[i];
    }

    global->undo_count++;
    global->redo_count = 0; // Clear redo stack on new action
}

/**
 * undo_save_insert - Save an insert action to undo stack
 * @global: Global state
 * @ch: Character inserted
 * @pos: Position where insertion occurred
 */
void undo_save_insert(struct global* global, char ch, uint32_t pos)
{
    undo_save_action(global, action_insert, &ch, 1, pos);
}

/**
 * undo_save_delete - Save a delete action to undo stack
 * @global: Global state
 * @ch: Character deleted
 * @pos: Position where deletion occurred
 */
void undo_save_delete(struct global* global, char ch, uint32_t pos)
{
    undo_save_action(global, action_delete, &ch, 1, pos);
}

/**
 * undo_perform - Perform an undo operation
 * @global: Global state
 *
 * Reverses the most recent action from the undo stack.
 * Saves the undone action to redo stack for possible redo.
 */
void undo_perform(struct global* global)
{
    if (global->undo_count == 0) return;

    struct action* act = &global->undo_stack[global->undo_count - 1];

    // Save to redo stack
    if (global->redo_count < UNDO_STACK_SIZE) {
        global->redo_stack[global->redo_count] = *act;
        global->redo_count++;
    }

    // Perform undo
    if (act->type == action_insert) {
        // Undo insert = delete the text that was inserted
        // Use batch delete for efficiency with large text
        pgb_delete_range(&global->text, act->pos, act->pos + act->len);
    } else if (act->type == action_delete) {
        // Undo delete = insert the text that was deleted
        // Move to position where it was deleted, then insert
        pgb_move_to_pos(&global->text, act->pos);
        // Use batch insert for efficiency with large text
        pgb_insert_str(&global->text, act->data, &global->arena);
    } else if (act->type == action_replace) {
        // Undo replace = restore the original text
        pgb_move_to_pos(&global->text, act->pos);
        // Delete new text using batch delete
        pgb_delete_range(&global->text, act->pos, act->pos + act->len);
        // Insert old text using batch insert
        pgb_move_to_pos(&global->text, act->pos);
        pgb_insert_str(&global->text, act->old_data, &global->arena);
    }

    global->undo_count--;
}

/**
 * redo_perform - Perform a redo operation
 * @global: Global state
 *
 * Reapplies the most recent undone action.
 */
void redo_perform(struct global* global)
{
    if (global->redo_count == 0) return;

    struct action* act = &global->redo_stack[global->redo_count - 1];

    // Perform redo
    if (act->type == action_insert) {
        // Redo insert = insert the text back at original position
        pgb_move_to_pos(&global->text, act->pos);
        // Use batch insert for efficiency with large text
        pgb_insert_str(&global->text, act->data, &global->arena);
    } else if (act->type == action_delete) {
        // Redo delete = delete the text again
        // Use batch delete for efficiency with large text
        pgb_delete_range(&global->text, act->pos, act->pos + act->len);
    } else if (act->type == action_replace) {
        // Redo replace = apply the replacement again
        pgb_move_to_pos(&global->text, act->pos);
        // Delete old text using batch delete
        pgb_delete_range(&global->text, act->pos, act->pos + act->old_len);
        // Insert new text using batch insert
        pgb_move_to_pos(&global->text, act->pos);
        pgb_insert_str(&global->text, act->data, &global->arena);
    }

    // Move action back to undo stack
    global->redo_count--;
    global->undo_count++;
}

/**
 * undo_save_replace - Save a replace action to undo stack
 * @global: Global state
 * @new_text: New replacement text
 * @new_len: Length of new text
 * @old_text: Original text being replaced
 * @old_len: Length of old text
 * @pos: Position where replacement occurred
 */
void undo_save_replace(struct global* global, const char* new_text, uint32_t new_len,
                       const char* old_text, uint32_t old_len, uint32_t pos)
{
    if (global->undo_count >= UNDO_STACK_SIZE) return;

    struct action* act = &global->undo_stack[global->undo_count];
    act->type = action_replace;
    act->len = new_len;
    act->old_len = old_len;
    act->pos = pos;

    // Copy new text
    uint32_t new_copy_len = new_len < MAX_SEARCH_QUERY_LEN ? new_len : MAX_SEARCH_QUERY_LEN;
    for (uint32_t i = 0; i < new_copy_len; i++) {
        act->data[i] = new_text[i];
    }

    // Copy old text
    uint32_t old_copy_len = old_len < MAX_SEARCH_QUERY_LEN ? old_len : MAX_SEARCH_QUERY_LEN;
    for (uint32_t i = 0; i < old_copy_len; i++) {
        act->old_data[i] = old_text[i];
    }

    global->undo_count++;
    global->redo_count = 0; // Clear redo stack on new action
}

/**
 * undo_save_batch_insert - Save a batch insert action to undo stack
 * @global: Global state
 * @text: Text to insert
 * @len: Length of text
 * @pos: Position where insertion occurs
 */
void undo_save_batch_insert(struct global* global, const char* text, uint32_t len, uint32_t pos)
{
    undo_save_action(global, action_insert, text, len, pos);
}

/**
 * undo_save_batch_delete - Save a batch delete action to undo stack
 * @global: Global state
 * @text: Text that was deleted
 * @len: Length of text
 * @pos: Position where deletion occurred
 */
void undo_save_batch_delete(struct global* global, const char* text, uint32_t len, uint32_t pos)
{
    undo_save_action(global, action_delete, text, len, pos);
}

/**
 * undo_clear_history - Clear all undo/redo history
 * @global: Global state
 */
void undo_clear_history(struct global* global)
{
    global->undo_count = 0;
    global->redo_count = 0;
}

/**
 * undo_can_undo - Check if undo is available
 * @global: Global state
 * @return: true if undo is available, false otherwise
 */
bool undo_can_undo(struct global* global)
{
    return global->undo_count > 0;
}

/**
 * undo_can_redo - Check if redo is available
 * @global: Global state
 * @return: true if redo is available, false otherwise
 */
bool undo_can_redo(struct global* global)
{
    return global->redo_count > 0;
}

// ========== Search Functionality ==========

/**
 * search_init - Initialize search state
 * @global: Global state
 * 
 * Resets search state to inactive.
 */
void search_init(struct global* global)
{
    global->search_active = false;
    global->search_query[0] = '\0';
    global->search_pos = 0;
    global->search_match_count = 0;
    global->search_query_len = 0;
}

/**
 * search_scan - Scan buffer using Knuth-Morris-Pratt algorithm
 * @pgb: Buffer to search
 * @q: Query string
 * @qlen: Query length
 * @start: Start offset for search
 * @stop: Stop offset (exclusive)
 * @want_last: If true, find last match; if false, find first
 * @count: Optional pointer to store total match count
 * 
 * Returns: Position of found match, or (uint32_t)-1 if none found
 * 
 * Uses KMP pattern matching for O(n) search complexity.
 * Efficiently scans the paged buffer without flattening.
 */
static uint32_t search_scan(const struct paged_gap_buffer* pgb, const char* q,
                            uint32_t qlen, uint32_t start, uint32_t stop,
                            bool want_last, uint32_t* count)
{
    if (!pgb || !q || qlen == 0 || qlen > MAX_SEARCH_QUERY_LEN) return (uint32_t)-1;
    
    uint32_t pi[MAX_SEARCH_QUERY_LEN], j = 0, pos = 0, found = (uint32_t)-1;
    // Build prefix function for KMP
    for (uint32_t i = 1; i < qlen; i++) {
        while (j && q[i] != q[j]) j = pi[j - 1];
        if (q[i] == q[j]) j++;
        pi[i] = j;
    }
    // Scan each page (before and after gap)
    for (struct page* p = pgb->head; p; p = p->next) {
        uint32_t parts[2] = {p->gap_start, PAGE_CAPACITY};
        uint32_t begins[2] = {0, p->gap_end};
        for (int part = 0; part < 2; part++) {
            uint32_t end = parts[part], b = begins[part];
            if (part == 1 && p->gap_end == PAGE_CAPACITY) continue;
            for (uint32_t k = b; k < end; k++, pos++) {
                unsigned char c = (unsigned char)p->data[k];
                while (j && c != (unsigned char)q[j]) j = pi[j - 1];
                if (c == (unsigned char)q[j]) j++;
                if (j == qlen) {
                    uint32_t at = pos + 1 - qlen;
                    if (at >= start && at < stop) {
                        if (count) (*count)++;
                        if (found == (uint32_t)-1 || want_last) found = at;
                    }
                    j = pi[j - 1];
                }
            }
        }
    }
    return found;
}

/**
 * search_find - Find first occurrence of query
 * @global: Global state
 * @query: Search string
 * 
 * Performs search and moves cursor to first match.
 * Updates search state with match information.
 */
void search_find(struct global* global, const char* query)
{
    if (!global || !query || query[0] == '\0') {
        if (global) {
            global->search_active = false;
            global->search_match_count = 0;
            global->search_query_len = 0;
        }
        return;
    }

    strncpy(global->search_query, query, sizeof(global->search_query) - 1);
    global->search_query[sizeof(global->search_query) - 1] = '\0';
    global->search_query_len = (uint32_t)strlen(global->search_query);
    global->search_match_count = 0;
    uint32_t first_match = search_scan(&global->text, global->search_query,
                                       global->search_query_len, 0, UINT32_MAX,
                                       false, &global->search_match_count);

    global->search_active = (global->search_match_count > 0);
    global->search_pos = first_match;

    if (first_match != (uint32_t) -1) {
        pgb_move_to_pos(&global->text, first_match);
    }
}

/**
 * search_next - Move to next search match
 * @global: Global state
 * 
 * Finds and navigates to the next match after current position.
 * Wraps around to beginning if at end.
 */
void search_next(struct global* global)
{
    if (!global || !global->search_active || global->search_query[0] == '\0') return;

    char* query = global->search_query;
    uint32_t query_len = global->search_query_len;

    uint32_t start_pos;
    if (global->search_pos == (uint32_t) -1) {
        start_pos = 0;
    } else {
        start_pos = global->search_pos + query_len;
    }

    uint32_t next_pos = search_scan(&global->text, query, query_len, start_pos, UINT32_MAX, false, NULL);
    if (next_pos == (uint32_t)-1) next_pos = search_scan(&global->text, query, query_len, 0, start_pos, false, NULL);
    if (next_pos != (uint32_t) -1) {
        global->search_pos = next_pos;
        pgb_move_to_pos(&global->text, next_pos);
    }
}

/**
 * search_prev - Move to previous search match
 * @global: Global state
 * 
 * Finds and navigates to the previous match before current position.
 * Wraps around to end if at beginning.
 */
void search_prev(struct global* global)
{
    if (!global || !global->search_active || global->search_query[0] == '\0') return;

    char* query = global->search_query;
    uint32_t prev_pos = search_scan(&global->text, query, global->search_query_len, 0, global->search_pos, true, NULL);
    if (prev_pos == (uint32_t)-1) prev_pos = search_scan(&global->text, query, global->search_query_len, global->search_pos, UINT32_MAX, true, NULL);

    if (prev_pos != (uint32_t) -1) {
        global->search_pos = prev_pos;
        pgb_move_to_pos(&global->text, prev_pos);
    }
}
