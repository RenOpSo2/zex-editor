#include "input.h"
#include "nodes.h"
#include "file.h"
#include "term.h"
#include "global.h"
#include "config.h"
#include "draw.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Escape sequence parser
// Handles plain arrows (\x1b[A-D), Shift+arrows (\x1b[1;2A-D),
// and X10 mouse click events (\x1b[M + 3 bytes).
// ---------------------------------------------------------------------------
typedef enum {
    esc_none,
    esc_got_esc,     // received \x1b
    esc_got_bracket, // received \x1b[
    esc_got_1,       // received \x1b[1
    esc_got_semi,    // received \x1b[1;
    esc_got_2,       // received \x1b[1;2
    esc_got_M,       // received \x1b[M - waiting for button byte
    esc_got_M_btn,   // got button byte - waiting for x byte
    esc_got_M_x,     // got x byte - waiting for y byte
} esc_state;

static esc_state esc = esc_none;
static unsigned char mouse_btn_byte = 0;
static unsigned char mouse_x_byte = 0;

// Search mode state
static enum bool search_mode_active = false;
static char search_input[256] = {0};
static uint32_t search_input_len = 0;

// ---------------------------------------------------------------------------
// Mouse click helper — move cursor to screen (col, row)
// ---------------------------------------------------------------------------
static void mouse_click_move(struct global* global, uint32_t screen_col, uint32_t screen_row) {
    // Row 1 = status bar, Row 2+ = text area
    if (screen_row < 2) return;

    uint32_t scroll_offset = draw_get_scroll_offset();
    uint32_t target_line = (screen_row - 2) + scroll_offset;
    int gutter = config_get_bool("show_line_numbers", 1) ? 6 : 0;
    int target_col = (int)screen_col - 1 - gutter;
    if (target_col < 0) target_col = 0;

    // Walk the buffer to find the byte offset for (target_line, target_col)
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global->text);

    uint32_t line = 0;
    uint32_t col = 0;
    uint32_t pos = 0;

    // Skip to target line
    while (buffer[pos] != '\0' && line < target_line) {
        if (buffer[pos] == '\n') line++;
        pos++;
    }

    // Now walk columns on the target line, accounting for tabs
    while (buffer[pos] != '\0' && buffer[pos] != '\n' && (int)col < target_col) {
        if (buffer[pos] == '\t') {
            int tab_size = (int)config_get_number("tabsize", 4);
            col += tab_size - (col % tab_size);
        } else {
            col++;
        }
        pos++;
    }

    pgb_move_to_pos(&global->text, pos);
}

// ---------------------------------------------------------------------------
// Auto-indent helper
// ---------------------------------------------------------------------------
static void auto_indent(struct global* g) {
    uint32_t cursor = pgb_cursor_pos(&g->text);
    if (cursor < 2) return; // Need at least the newline and one character before it
    
    char buf[buf_capacity];
    pgb_to_str(buf, sizeof(buf), &g->text);
    
    // The newly inserted newline is at cursor - 1
    uint32_t total_size = strlen(buf);
    if (cursor - 1 >= total_size || buf[cursor - 1] != '\n') {
        return;
    }
    
    // Walk backwards from cursor - 2 to find start of the previous line
    int prev_line_start = (int)cursor - 2;
    while (prev_line_start > 0 && buf[prev_line_start - 1] != '\n') {
        prev_line_start--;
    }
    
    // Now count and collect the leading whitespace of that previous line
    char indent_str[256];
    uint32_t indent_len = 0;
    int i = prev_line_start;
    while (i < (int)cursor - 1 && indent_len < sizeof(indent_str) - 1) {
        char ch = buf[i];
        if (ch == ' ' || ch == '\t') {
            indent_str[indent_len++] = ch;
            i++;
        } else {
            break;
        }
    }
    indent_str[indent_len] = '\0';
    
    // Insert the indentation at the current cursor position
    for (uint32_t j = 0; j < indent_len; j++) {
        pgb_insert(&g->text, indent_str[j], &g->arena);
        undo_save_insert(g, indent_str[j], cursor + j);
    }
}

// ---------------------------------------------------------------------------
// Selection helpers
// ---------------------------------------------------------------------------
static void sel_begin(struct global* g) {
    if (!g->has_selection) {
        g->sel_anchor = pgb_cursor_pos(&g->text);
        g->has_selection = true;
    }
}

static void sel_clear(struct global* g) {
    g->has_selection = false;
}

// Return normalised [start, end) for the current selection.
static void sel_range(struct global* g, uint32_t* from, uint32_t* to) {
    uint32_t cursor = pgb_cursor_pos(&g->text);
    if (g->sel_anchor <= cursor) {
        *from = g->sel_anchor;
        *to   = cursor;
    } else {
        *from = cursor;
        *to   = g->sel_anchor;
    }
}

// Delete the selected region, position cursor at from, clear selection.
static void sel_delete(struct global* g) {
    uint32_t from, to;
    sel_range(g, &from, &to);
    pgb_delete_range(&g->text, from, to);
    sel_clear(g);
}

// ---------------------------------------------------------------------------
// Input handler
// ---------------------------------------------------------------------------
enum result input_update(struct global* global) {
    char buf[term_capacity];
    uint32_t n = term_read(buf);

    for (uint32_t i = 0; i < n; i++) {
        unsigned char ch = (unsigned char)buf[i];

        // ---- Search mode handling --------------------------------------------
        if (search_mode_active) {
            if (ch == 0x1b) { // ESC - exit search mode
                search_mode_active = false;
                search_input_len = 0;
                search_input[0] = '\0';
                pgb_replace_str(&global->msg, "Search cancelled.", &global->arena);
                continue;
            } else if (ch == '\r' || ch == '\n') { // Enter - perform search
                search_mode_active = false;
                search_input[search_input_len] = '\0';
                search_find(global, search_input);
                char msg[512];
                if (global->search_match_count > 0) {
                    snprintf(msg, sizeof(msg), "Found %d matches for '%s'", 
                             global->search_match_count, search_input);
                } else {
                    snprintf(msg, sizeof(msg), "No matches for '%s'", search_input);
                }
                pgb_replace_str(&global->msg, msg, &global->arena);
                search_input_len = 0;
                search_input[0] = '\0';
                continue;
            } else if (ch == '\b' || ch == 127) { // Backspace
                if (search_input_len > 0) {
                    search_input_len--;
                    search_input[search_input_len] = '\0';
                }
                continue;
            } else if (ch >= 32 && ch < 127) { // Printable character
                if (search_input_len < sizeof(search_input) - 1) {
                    search_input[search_input_len++] = ch;
                    search_input[search_input_len] = '\0';
                }
                continue;
            }
            // Fall through to normal processing for other keys
        }

        // ---- Escape sequence state machine --------------------------------
        if (esc == esc_none && ch == 0x1b) {
            esc = esc_got_esc;
            continue;
        }
        if (esc == esc_got_esc) {
            if (ch == '[') { esc = esc_got_bracket; continue; }
            esc = esc_none; continue; // unknown, discard
        }
        if (esc == esc_got_bracket) {
            // Plain arrow
            if (ch == 'A') { sel_clear(global); pgb_move_up(&global->text);    esc = esc_none; continue; }
            if (ch == 'B') { sel_clear(global); pgb_move_down(&global->text);  esc = esc_none; continue; }
            if (ch == 'C') { sel_clear(global); pgb_move_right(&global->text); esc = esc_none; continue; }
            if (ch == 'D') { sel_clear(global); pgb_move_left(&global->text);  esc = esc_none; continue; }
            // Start of mouse reporting sequence \x1b[M
            if (ch == 'M') { esc = esc_got_M; continue; }
            // Start of modifier sequence \x1b[1
            if (ch == '1') { esc = esc_got_1; continue; }
            esc = esc_none; continue;
        }
        if (esc == esc_got_1) {
            if (ch == ';') { esc = esc_got_semi; continue; }
            esc = esc_none; continue;
        }
        if (esc == esc_got_semi) {
            if (ch == '2') { esc = esc_got_2; continue; } // modifier = Shift
            esc = esc_none; continue;
        }
        if (esc == esc_got_2) {
            // Shift + Arrow → extend selection
            esc = esc_none;
            sel_begin(global); // set anchor if first shift-arrow
            switch (ch) {
                case 'A': pgb_move_up(&global->text);    continue;
                case 'B': pgb_move_down(&global->text);  continue;
                case 'C': pgb_move_right(&global->text); continue;
                case 'D': pgb_move_left(&global->text);  continue;
                default:  continue;
            }
        }
        if (esc == esc_got_M) {
            mouse_btn_byte = ch;
            esc = esc_got_M_btn;
            continue;
        }
        if (esc == esc_got_M_btn) {
            mouse_x_byte = ch;
            esc = esc_got_M_x;
            continue;
        }
        if (esc == esc_got_M_x) {
            unsigned char mouse_y_byte = ch;
            esc = esc_none;
            
            // X10 reporting coordinates are 1-based and offset by 32
            int btn = mouse_btn_byte - 32;
            int col = mouse_x_byte - 32;
            int row = mouse_y_byte - 32;
            
            if (btn == 0 && config_get_bool("mouse", 1)) {
                sel_clear(global);
                mouse_click_move(global, col, row);
            }
            continue;
        }

        // ---- Ctrl+Q: quit -------------------------------------------------
        if (ch == CTRL_KEY('q')) {
            return err;
        }

        // ---- Ctrl+S: save -------------------------------------------------
        if (ch == CTRL_KEY('s')) {
            sel_clear(global);
            if (global->filepath[0] != '\0') {
                if (file_write(global->filepath, &global->text) == ok) {
                    pgb_replace_str(&global->msg, "Saved.", &global->arena);
                } else {
                    pgb_replace_str(&global->msg, "Save failed!", &global->arena);
                }
            } else {
                pgb_replace_str(&global->msg, "No filepath — pass one as argument.", &global->arena);
            }
            continue;
        }

        // ---- Ctrl+U: undo -------------------------------------------------
        if (ch == CTRL_KEY('u')) {
            sel_clear(global);
            undo_perform(global);
            pgb_replace_str(&global->msg, "Undo.", &global->arena);
            continue;
        }

        // ---- Ctrl+R: refresh screen (rendo fix) -----------------------------
        if (ch == CTRL_KEY('r')) {
            sel_clear(global);
            pgb_replace_str(&global->msg, "Screen refreshed.", &global->arena);
            // Force redraw by calling draw_update directly
            // The draw will happen in editor_update, but we clear the message first
            continue;
        }

        // ---- Ctrl+C: copy -------------------------------------------------
        if (ch == CTRL_KEY('c')) {
            if (global->has_selection) {
                uint32_t from, to;
                sel_range(global, &from, &to);
                pgb_copy_range(&global->clipboard, &global->text, from, to, &global->arena);
                pgb_replace_str(&global->msg, "Copied.", &global->arena);
                sel_clear(global);
            }
            continue;
        }

        // ---- Ctrl+X: cut --------------------------------------------------
        if (ch == CTRL_KEY('x')) {
            if (global->has_selection) {
                uint32_t from, to;
                sel_range(global, &from, &to);
                pgb_copy_range(&global->clipboard, &global->text, from, to, &global->arena);
                pgb_delete_range(&global->text, from, to);
                pgb_replace_str(&global->msg, "Cut.", &global->arena);
                sel_clear(global);
            }
            continue;
        }

        // ---- Ctrl+V: paste ------------------------------------------------
        if (ch == CTRL_KEY('v')) {
            // If something is selected, replace it with clipboard.
            if (global->has_selection) {
                sel_delete(global);
            }
            struct page* p = global->clipboard.head;
            while (p) {
                for (uint32_t j = 0; j < p->gap_start; j++)
                    pgb_insert(&global->text, p->data[j], &global->arena);
                for (uint32_t j = p->gap_end; j < PAGE_CAPACITY; j++)
                    pgb_insert(&global->text, p->data[j], &global->arena);
                p = p->next;
            }
            pgb_replace_str(&global->msg, "Pasted.", &global->arena);
            continue;
        }

        // ---- Ctrl+A: select all -------------------------------------------
        if (ch == CTRL_KEY('a')) {
            pgb_move_to_pos(&global->text, 0);
            global->sel_anchor = 0;
            global->has_selection = true;
            // Move cursor to end
            while (1) {
                struct page* p = global->text.active_page;
                if (p->gap_end == PAGE_CAPACITY && !p->next) break;
                pgb_move_right(&global->text);
            }
            pgb_replace_str(&global->msg, "Selected all.", &global->arena);
            continue;
        }

        // ---- Ctrl+F: search ---------------------------------------------------
        if (ch == CTRL_KEY('f')) {
            sel_clear(global);
            search_mode_active = true;
            search_input_len = 0;
            search_input[0] = '\0';
            pgb_replace_str(&global->msg, "Search: ", &global->arena);
            continue;
        }

        // ---- Ctrl+N: search next ---------------------------------------------
        if (ch == CTRL_KEY('n')) {
            sel_clear(global);
            search_next(global);
            continue;
        }

        // ---- Ctrl+P: search previous -----------------------------------------
        if (ch == CTRL_KEY('p')) {
            sel_clear(global);
            search_prev(global);
            continue;
        }

        // ---- Ctrl+Y: redo ----------------------------------------------------
        if (ch == CTRL_KEY('y')) {
            sel_clear(global);
            redo_perform(global);
            pgb_replace_str(&global->msg, "Redo.", &global->arena);
            continue;
        }

        // ---- Regular text input -------------------------------------------
        sel_clear(global); // any non-special key clears selection unless it's a replace

        if (ch == '\b' || ch == 127) {
            uint32_t cursor_before = pgb_cursor_pos(&global->text);
            if (cursor_before > 0) {
                // The character being removed is the one immediately before
                // the cursor, at index (cursor_before - 1) — not the last
                // character of the whole document. That is also the correct
                // position to record for undo, since after the delete the
                // cursor rests exactly there.
                char buffer[buf_capacity];
                pgb_to_str(buffer, sizeof(buffer), &global->text);
                uint32_t del_pos = cursor_before - 1;
                char deleted_char = buffer[del_pos];

                pgb_delete(&global->text);
                undo_save_delete(global, deleted_char, del_pos);
            }
        } else if (ch == '\r') {
            uint32_t pos = pgb_cursor_pos(&global->text);
            pgb_insert(&global->text, '\n', &global->arena);
            undo_save_insert(global, '\n', pos);
            if (config_get_bool("auto_indent", 1)) {
                auto_indent(global);
            }
        } else if (ch >= 32 || ch == '\t' || ch == '\n') {
            uint32_t pos = pgb_cursor_pos(&global->text);
            pgb_insert(&global->text, (char)ch, &global->arena);
            undo_save_insert(global, (char)ch, pos);
        }
    }
    return ok;
}
