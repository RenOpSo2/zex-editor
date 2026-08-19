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
    struct page* p = pgb->active_page;
    if (p->gap_start > 0) {
        p->gap_start--;  // Simple case: gap expands left
    } else if (p->prev) {
        // Cursor at start of page - need to pull content from previous page
        pgb->active_page = p->prev;
        p = pgb->active_page;
        // Move all content after gap to before gap (compact page)
        while (p->gap_end < PAGE_CAPACITY) {
            p->data[p->gap_start++] = p->data[p->gap_end++];
        }
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

/**
 * pgb_move_left - Move cursor one character left
 * @pgb: Paged gap buffer
 * 
 * Moves gap left by swapping character before gap with gap position.
 * If at page start, moves to previous page's end.
 */
void pgb_move_left(struct paged_gap_buffer* pgb)
{
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
        // Compact current page (move all content after gap to before gap)
        while (p->gap_end < PAGE_CAPACITY) {
            p->data[p->gap_start++] = p->data[p->gap_end++];
        }
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
    struct page* p = pgb->active_page;
    if (p->gap_end < PAGE_CAPACITY) {
        // Move gap right: swap character after gap into gap
        p->data[p->gap_start++] = p->data[p->gap_end++];
    } else if (p->next) {
        // Move to next page
        pgb->active_page = p->next;
        p = pgb->active_page;
        // Compact current page (move all content before gap to after gap)
        while (p->gap_start > 0) {
            p->gap_end--;
            p->gap_start--;
            p->data[p->gap_end] = p->data[p->gap_start];
        }
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
static uint32_t get_current_column(struct paged_gap_buffer* pgb)
{
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
static uint32_t get_line_length(struct paged_gap_buffer* pgb)
{
    uint32_t len = 0;
    while (1) {
        struct page* p = pgb->active_page;
        if (p->gap_end == PAGE_CAPACITY && !p->next) break;
        pgb_move_right(pgb);
        p = pgb->active_page;
        if (p->data[p->gap_start - 1] == '\n') {
            pgb_move_left(pgb);
            break;
        }
        len++;
    }
    return len;
}

/**
 * pgb_move_up - Move cursor up one line
 * @pgb: Paged gap buffer
 * 
 * Preserves horizontal position as much as possible.
 * If current column exceeds previous line length, snaps to line end.
 */
void pgb_move_up(struct paged_gap_buffer* pgb)
{
    uint32_t col = get_current_column(pgb);

    // If already at first line, restore position and return
    if (pgb->active_page->gap_start == 0 && !pgb->active_page->prev) {
        for (uint32_t i = 0; i < col; i++) {
            pgb_move_right(pgb);
        }
        return;
    }

    // Move to previous line
    move_to_line_start(pgb);
    pgb_move_left(pgb);

    // Get previous line length
    uint32_t prev_line_len = get_line_length(pgb);

    // Move to target column (snap to end if needed)
    uint32_t target = col < prev_line_len ? col : prev_line_len;
    for (uint32_t i = 0; i < target; i++) {
        pgb_move_right(pgb);
    }
}

/**
 * pgb_move_down - Move cursor down one line
 * @pgb: Paged gap buffer
 * 
 * Preserves horizontal position as much as possible.
 * If at last line, does nothing.
 */
void pgb_move_down(struct paged_gap_buffer* pgb)
{
    uint32_t col = get_current_column(pgb);

    // Restore position to start of line
    for (uint32_t i = 0; i < col; i++) {
        pgb_move_right(pgb);
    }

    // Move to next line
    while (1) {
        struct page* p = pgb->active_page;
        if (p->gap_end == PAGE_CAPACITY && !p->next) return;  // At end of buffer
        pgb_move_right(pgb);
        p = pgb->active_page;
        if (p->data[p->gap_start - 1] == '\n') {
            break;
        }
    }

    // Get next line length and move to target column
    uint32_t next_line_len = get_line_length(pgb);
    uint32_t target = col < next_line_len ? col : next_line_len;
    for (uint32_t i = 0; i < target; i++) {
        pgb_move_right(pgb);
    }
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
    // Move to start first
    while (pgb->active_page->prev) {
        pgb->active_page = pgb->active_page->prev;
    }
    // Compact first page completely
    while (pgb->active_page->gap_start > 0) {
        pgb->active_page->gap_end--;
        pgb->active_page->gap_start--;
        pgb->active_page->data[pgb->active_page->gap_end] =
            pgb->active_page->data[pgb->active_page->gap_start];
    }
    // Advance right by target steps
    for (uint32_t i = 0; i < target; i++) {
        struct page* p = pgb->active_page;
        if (p->gap_end == PAGE_CAPACITY && !p->next) break;
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
    if (from >= to) return;
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
 * @type: Action type (insert or delete)
 * @data: Data associated with action
 * @len: Length of data
 * @pos: Position where action occurred
 * 
 * Internal helper for undo/redo tracking.
 */
static void undo_save_action(struct global* global, enum action_type type, const char* data, uint32_t len, uint32_t pos)
{
    if (global->undo_count >= UNDO_STACK_SIZE) return;

    struct action* act = &global->undo_stack[global->undo_count];
    act->type = type;
    act->len = len;
    act->pos = pos;

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
                            enum bool want_last, uint32_t* count)
{
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
    if (query[0] == '\0') {
        global->search_active = false;
        global->search_match_count = 0;
        global->search_query_len = 0;
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
    if (!global->search_active || global->search_query[0] == '\0') return;

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
    if (!global->search_active || global->search_query[0] == '\0') return;

    char* query = global->search_query;
    uint32_t prev_pos = search_scan(&global->text, query, global->search_query_len, 0, global->search_pos, true, NULL);
    if (prev_pos == (uint32_t)-1) prev_pos = search_scan(&global->text, query, global->search_query_len, global->search_pos, UINT32_MAX, true, NULL);

    if (prev_pos != (uint32_t) -1) {
        global->search_pos = prev_pos;
        pgb_move_to_pos(&global->text, prev_pos);
    }
}
