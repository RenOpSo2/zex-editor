#include "draw.h"
#include "nodes.h"
#include "render_buffer.h"
#include "syntax.h"
#include "config.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

static RenderBuffer rb;
static uint32_t last_scroll_offset = 0;

#define RB_ESC(s) rb_append(&rb, s, sizeof(s) - 1)

// Safe compile-time length for string literals — never miscounts again.
#define ESC(s) (s), (sizeof(s) - 1)

/**
 * Expand tab characters to spaces
 * 
 * @param input     Source string
 * @param input_len Length of input
 * @param output    Output buffer (must be pre-allocated)
 * @param output_size Size of output buffer
 * @return Number of characters written (excluding null terminator)
 */
static int expand_tabs(const char* input, int input_len, char* output, int output_size, int tab_size)
{
    int col = 0;
    int out_pos = 0;

    for (int i = 0; i < input_len && out_pos < output_size - 1; i++) {
        if (input[i] == '\t') {
            int num_spaces = tab_size - (col % tab_size);
            for (int s = 0; s < num_spaces && out_pos < output_size - 1; s++) {
                output[out_pos++] = ' ';
                col++;
            }
        } else {
            output[out_pos++] = input[i];
            col++;
        }
    }
    output[out_pos] = '\0';
    return out_pos;
}

// Helper: Apply syntax highlighting based on language
static void apply_syntax_highlighting(const char* line, int line_len, int language)
{
    if (language == 1) {
        // C/C++
        char* highlighted = syntax_highlight_line((char*)line, line_len);
        if (highlighted) {
            rb_append(&rb, highlighted, strlen(highlighted));
            free(highlighted);
        }
    } else if (language == 2) {
        // Python
        char* highlighted = syntax_highlight_python_line((char*)line, line_len);
        if (highlighted) {
            rb_append(&rb, highlighted, strlen(highlighted));
            free(highlighted);
        }
    } else {
        // No highlighting - just output plain text
        rb_append(&rb, line, line_len);
    }
}

// Helper: Draw line number if enabled
static void draw_line_number(uint32_t line_num)
{
    if (config_get_bool("show_line_numbers", 1)) {
        RB_ESC("\x1b[0m\x1b[38;5;244m");
        char line_num_str[16];
        int line_num_len = snprintf(line_num_str, sizeof(line_num_str), "%5d ", line_num + 1);
        if (line_num_len > 0) {
            rb_append(&rb, line_num_str, line_num_len);
        }
        RB_ESC("\x1b[0m\x1b[39;49m");
    }
}

void draw_init(void)
{
    rb_init(&rb);

    RB_ESC("\x1b[?1049h");
    RB_ESC("\x1b[0m\x1b[39;49m");
    RB_ESC("\x1b[2J");
    RB_ESC("\x1b[H");
    rb_flush(&rb);
}

static void draw_cursor_home(void)
{
    RB_ESC("\x1b[0m\x1b[39;49m\x1b[H\x1b[2J");
}

static void compute_cursor_and_totals(const char* full_buffer, uint32_t cursor_pos, uint32_t* out_line, uint32_t* out_col, uint32_t* out_total_lines)
{
    uint32_t temp_line = 0;
    uint32_t temp_col = 0;
    uint32_t buffer_len = strlen(full_buffer);

    if (cursor_pos > buffer_len) cursor_pos = buffer_len;

    for (uint32_t i = 0; i < cursor_pos && full_buffer[i] != '\0'; i++) {
        if (full_buffer[i] == '\n') {
            temp_line++;
            temp_col = 0;
        } else if (full_buffer[i] == '\t') {
            int tab_size = (int)config_get_number("tabsize", 4);
            temp_col += tab_size - (temp_col % tab_size);
        } else {
            temp_col++;
        }
    }

    *out_line = temp_line;
    *out_col = temp_col;

    uint32_t total = 0;
    for (uint32_t i = 0; full_buffer[i] != '\0'; i++) {
        if (full_buffer[i] == '\n') total++;
    }
    if (buffer_len != 0 && full_buffer[buffer_len - 1] != '\n') total++;
    *out_total_lines = total;
}

static uint32_t compute_scroll_offset(uint32_t cursor_line, uint32_t size_y)
{
    uint32_t scroll_offset = last_scroll_offset;
    if (cursor_line < scroll_offset) {
        scroll_offset = cursor_line;
    } else if (cursor_line >= scroll_offset + size_y) {
        scroll_offset = cursor_line - size_y + 1;
    }
    last_scroll_offset = scroll_offset;
    return scroll_offset;
}

// Render a single logical line (given start pointer and length)
static void render_logical_line(const char* line_start, int raw_len, uint32_t line_num, int language)
{
    draw_line_number(line_num);

    static char expanded_line[buf_capacity * 4];
    int tab_size = (int)config_get_number("tabsize", 4);
    int expanded_len = expand_tabs(line_start, raw_len, expanded_line, sizeof(expanded_line), tab_size);

    apply_syntax_highlighting(expanded_line, expanded_len, language);
    RB_ESC("\x1b[K\r\n");
}

// Walk the full_buffer and render only the visible lines determined by scroll_offset
static void render_visible_lines(const char* full_buffer, uint32_t size_y, uint32_t scroll_offset, int language)
{
    uint32_t rendered = 0;
    uint32_t buffer_line = 0;
    uint32_t pos = 0;
    uint32_t line_start_idx = 0;

    while (full_buffer[pos] != '\0' && rendered < size_y) {
        if (full_buffer[pos] == '\n') {
            if (buffer_line >= scroll_offset) {
                render_logical_line(full_buffer + line_start_idx, (int)(pos - line_start_idx), buffer_line, language);
                rendered++;
            }
            buffer_line++;
            line_start_idx = pos + 1;
        }
        pos++;
    }

    // Last line without newline
    if (line_start_idx < pos && buffer_line >= scroll_offset && rendered < size_y) {
        render_logical_line(full_buffer + line_start_idx, (int)(pos - line_start_idx), buffer_line, language);
        rendered++;
    }

    for (; rendered < size_y; rendered++) {
        RB_ESC("\x1b[K\r\n");
    }
}

static void position_cursor(uint32_t cursor_line, uint32_t cursor_col, uint32_t scroll_offset)
{
    uint32_t visible_cursor_line = cursor_line;
    if (visible_cursor_line >= scroll_offset) visible_cursor_line -= scroll_offset; else visible_cursor_line = 0;

    char cursor_seq[64];
    int gutter_offset = config_get_bool("show_line_numbers", 1) ? 7 : 1;
    int len = snprintf(cursor_seq, sizeof(cursor_seq), "\x1b[%d;%dH", visible_cursor_line + 2, (int)cursor_col + gutter_offset);
    if (len > 0 && len < (int)sizeof(cursor_seq)) {
        rb_append(&rb, cursor_seq, len);
    }
    RB_ESC("\x1b[?25h");
}

static void draw_text(const struct paged_gap_buffer* pgb, uint32_t size_y, const char* filepath)
{
    char full_buffer[buf_capacity];
    pgb_to_str(full_buffer, sizeof(full_buffer), pgb);

    uint32_t cursor_pos = pgb_cursor_pos(pgb);

    RB_ESC("\x1b[0m\x1b[39;49m");

    uint32_t cursor_line = 0, cursor_col = 0, total_lines = 0;
    compute_cursor_and_totals(full_buffer, cursor_pos, &cursor_line, &cursor_col, &total_lines);

    uint32_t scroll_offset = compute_scroll_offset(cursor_line, size_y);

    int language = 0;
    if (filepath && filepath[0] != '\0') language = syntax_get_language(filepath);

    render_visible_lines(full_buffer, size_y, scroll_offset, language);

    position_cursor(cursor_line, cursor_col, scroll_offset);
}

static void draw_status(struct global* global)
{
    /* Status bar: dark bg, light fg. */
    RB_ESC("\x1b[0m\x1b[48;5;235m\x1b[38;5;250m");

    RB_ESC(" zex | ");

    if (global->filepath[0] != '\0') {
        rb_append(&rb, global->filepath, strlen(global->filepath));
    } else {
        RB_ESC("[No file]");
    }

    /* Dim hints. */
    RB_ESC("  \x1b[38;5;244mCtrl+S: Save  Ctrl+Q: Quit  Ctrl+F: Search  Ctrl+R: Refresh");

    /* Flash message in yellow (if any). */
    uint32_t msg_len = 0;
    struct page *mp = global->msg.head;

    while (mp) {
        msg_len += mp->gap_start + (PAGE_CAPACITY - mp->gap_end);
        mp = mp->next;
    }

    if (msg_len > 0) {
        RB_ESC("  \x1b[38;5;220m");

        struct page *p = global->msg.head;

        while (p) {
            if (p->gap_start > 0) {
                rb_append(&rb, p->data, p->gap_start);
            }

            uint32_t r = PAGE_CAPACITY - p->gap_end;
            if (r > 0) {
                rb_append(&rb, p->data + p->gap_end, r);
            }

            p = p->next;
        }
    }

    /* Reset + erase rest of status line + newline. */
    RB_ESC("\x1b[0m\x1b[39;49m\x1b[K\n");
}

void draw_update(struct global* global)
{
    rb_clear(&rb);

    draw_cursor_home();
    draw_status(global);
    draw_text(&global->text, global->term.ws.ws_row - 2, global->filepath);

    rb_flush(&rb);
}

void draw_deinit(void)
{
    rb_clear(&rb);

    RB_ESC("\x1b[?25h");
    RB_ESC("\x1b[0m\x1b[39;49m");
    RB_ESC("\x1b[?1049l");

    rb_flush(&rb);
}

uint32_t draw_get_scroll_offset(void)
{
    return last_scroll_offset;
}
