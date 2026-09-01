#include "draw.h"
#include "nodes.h"
#include "render_buffer.h"
#include "syntax.h"
#include "config.h"
#include "dispwidth.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DRAW_FALLBACK_ROWS 24u
#define DRAW_FALLBACK_COLS 80u

static RenderBuffer rb, line_disp;
static uint32_t last_scroll_offset = 0;

#define RB_ESC(s) ((void)rb_append(&rb, (s), sizeof(s) - 1))

static uint32_t draw_tab_size(void)
{
    int ts = (int)config_get_number("tabsize", 4);
    return (uint32_t)(ts < 1 ? 1 : ts > 16 ? 16 : ts);
}

uint32_t draw_gutter_width(void)
{
    return config_get_bool("show_line_numbers", 1) ? 6u : 0u;
}

static void append_sanitized(RenderBuffer* out, const char* s, size_t len, uint32_t* budget)
{
    if (!out || !s || !budget) return;
    size_t i = 0;
    while (i < len && *budget > 0) {
        unsigned char ch[4];
        size_t got = 0;
        ch[got++] = (unsigned char)s[i++];
        int extra = utf8_trail_count(ch[0]);
        for (int k = 0; k < extra && i < len; k++) {
            ch[got++] = (unsigned char)s[i++];
        }
        unsigned char b = ch[0];
        if (b < 32 || b == 127) {
            if (*budget < 2) break;
            char pair[2] = {'^', (char)(b == 127 ? '?' : b + '@')};
            if (rb_append(out, pair, 2) != ok) return;
            *budget -= 2;
        } else {
            uint32_t cp;
            (void)utf8_decode(ch, got, &cp);
            uint32_t w = codepoint_width(cp);
            if (w > *budget) break;
            if (rb_append(out, (const char*)ch, got) != ok) return;
            *budget -= w;
        }
    }
}

static void draw_line_number(uint32_t line_num)
{
    if (!config_get_bool("show_line_numbers", 1)) return;
    // Modern line number styling with better contrast
    RB_ESC("\x1b[0m\x1b[38;5;238m"); // Darker gray for better readability
    RB_ESC("\x1b[1m"); // Bold for better visibility
    char buf[16];
    int n = snprintf(buf, sizeof(buf), "%5u ", (unsigned)(line_num + 1u));
    if (n > 0) {
        size_t out = ((size_t)n < sizeof(buf)) ? (size_t)n : sizeof(buf);
        if (out > draw_gutter_width()) out = draw_gutter_width();
        rb_append(&rb, buf, out);
    }
    RB_ESC("\x1b[0m\x1b[39;49m");
}

void draw_init(void)
{
    rb_init(&rb);
    rb_init(&line_disp);
    // Modern terminal initialization with extended features
    RB_ESC("\x1b[?1049h"); // Alternate screen buffer
    RB_ESC("\x1b[0m");     // Reset all attributes
    RB_ESC("\x1b[39;49m"); // Default colors
    RB_ESC("\x1b[2J");     // Clear screen
    RB_ESC("\x1b[H");      // Home cursor
    RB_ESC("\x1b[?25l");   // Hide cursor initially
    rb_flush(&rb);
}

static void draw_cursor_home(void)
{
    RB_ESC("\x1b[0m");     // Reset attributes
    RB_ESC("\x1b[39;49m"); // Default colors
    RB_ESC("\x1b[H");      // Home cursor
    RB_ESC("\x1b[2J");     // Clear screen
}

struct doc_stats {
    uint32_t cur_line, cur_col, total_lines;
};

static void compute_doc_stats(const struct paged_gap_buffer* pgb, uint32_t cursor_pos, struct doc_stats* st)
{
    uint32_t tab_size = draw_tab_size();
    uint32_t line = 0, col = 0, newlines = 0, len = 0;
    int ends_with_nl = 1;
    struct pgb_reader it;
    pgb_reader_init(&it, pgb);
    int c;
    unsigned char ch[4];
    while ((c = pgb_reader_next(&it)) >= 0) {
        size_t got = 0;
        ch[got++] = (unsigned char)c;
        int extra = utf8_trail_count((unsigned char)c);
        for (int k = 0; k < extra; k++) {
            int d = pgb_reader_next(&it);
            if (d < 0) break;
            ch[got++] = (unsigned char)d;
        }
        if (len < cursor_pos) {
            if (ch[0] == '\n') {
                line++;
                col = 0;
            } else {
                size_t consumed;
                col += char_width(ch, got, col, tab_size, &consumed);
            }
        }
        if (ch[0] == '\n') {
            newlines++;
            ends_with_nl = 1;
        } else {
            ends_with_nl = 0;
        }
        len += (uint32_t)got;
    }
    st->cur_line = line;
    st->cur_col = col;
    st->total_lines = (len == 0) ? 0 : newlines + (ends_with_nl ? 0u : 1u);
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

static void line_store_char(const unsigned char* ch, size_t n, uint32_t* disp_cols, uint32_t tab_size, uint32_t width)
{
    if (*disp_cols >= width) return;
    unsigned char b = ch[0];
    if (b == '\t') {
        uint32_t spaces = tab_size - (*disp_cols % tab_size);
        while (spaces-- > 0) {
            if (*disp_cols >= width) break;
            if (rb_append_char(&line_disp, ' ') != ok) return;
            (*disp_cols)++;
        }
        return;
    }
    if (b < 32 || b == 127) {
        if (width - *disp_cols < 2) return;
        char pair[2] = {'^', (char)(b == 127 ? '?' : b + '@')};
        if (rb_append(&line_disp, pair, 2) != ok) return;
        *disp_cols += 2;
        return;
    }
    uint32_t cp;
    (void)utf8_decode(ch, n, &cp);
    uint32_t w = codepoint_width(cp);
    if (*disp_cols + w > width) {
        *disp_cols = width;
        return;
    }
    if (rb_append(&line_disp, (const char*)ch, n) != ok) return;
    *disp_cols += w;
}

static void emit_visible_line(uint32_t line_num, int language)
{
    draw_line_number(line_num);
    if (language != 0 && line_disp.len > 0) {
        (void)rb_append_char(&line_disp, '\0');
        uint32_t body_len = (uint32_t)line_disp.len - 1u;
        char* hl = (language == 2) ? syntax_highlight_python_line(line_disp.data, body_len) : syntax_highlight_line(line_disp.data, body_len);
        if (hl) {
            rb_append(&rb, hl, strlen(hl));
            free(hl);
        } else {
            rb_append(&rb, line_disp.data, body_len);
        }
    } else {
        rb_append(&rb, line_disp.data, line_disp.len);
    }
    RB_ESC("\x1b[K\r\n");
}

static void render_visible_lines(const struct paged_gap_buffer* pgb, uint32_t rows, uint32_t scroll_offset, uint32_t width, int language)
{
    uint32_t tab_size = draw_tab_size();
    uint32_t rendered = 0, line_no = 0, disp_cols = 0;
    rb_clear(&line_disp);
    struct pgb_reader it;
    pgb_reader_init(&it, pgb);
    int c;
    unsigned char ch[4];
    
    // Skip lines before scroll_offset
    while (line_no < scroll_offset && (c = pgb_reader_next(&it)) >= 0) {
        if (c == '\n') {
            line_no++;
        }
    }
    
    // Now render visible lines
    while ((c = pgb_reader_next(&it)) >= 0 && rendered < rows) {
        if (c == '\n') {
            emit_visible_line(line_no, language);
            rendered++;
            line_no++;
            rb_clear(&line_disp);
            disp_cols = 0;
            continue;
        }
        size_t got = 0;
        ch[got++] = (unsigned char)c;
        int extra = utf8_trail_count((unsigned char)c);
        for (int k = 0; k < extra; k++) {
            int d = pgb_reader_next(&it);
            if (d < 0) break;
            ch[got++] = (unsigned char)d;
        }
        line_store_char(ch, got, &disp_cols, tab_size, width);
    }
    
    // Render last line if it doesn't end with newline
    if (line_disp.len > 0) {
        emit_visible_line(line_no, language);
        rendered++;
    }
    
    for (; rendered < rows; rendered++) {
        RB_ESC("\x1b[K\r\n");
    }
}

static void position_cursor(uint32_t cursor_line, uint32_t cursor_col, uint32_t scroll_offset, uint32_t cols, uint32_t rows)
{
    uint32_t vis_line = (cursor_line >= scroll_offset) ? cursor_line - scroll_offset : 0;
    if (rows > 0 && vis_line > rows - 1) vis_line = rows - 1;
    uint32_t gutter = draw_gutter_width();
    uint32_t avail = (cols > gutter) ? cols - gutter : 1;
    uint32_t disp_col = (cursor_col < avail) ? cursor_col : avail - 1;
    char seq[32];
    // Status bar is at line 0, text starts at line 1 (1-indexed)
    int n = snprintf(seq, sizeof(seq), "\x1b[%u;%uH", (unsigned)(vis_line + 2u), (unsigned)(disp_col + gutter + 1u));
    if (n > 0 && (size_t)n < sizeof(seq)) {
        rb_append(&rb, seq, (size_t)n);
    }
    RB_ESC("\x1b[?25h"); // Show cursor
}

static void draw_text(const struct paged_gap_buffer* pgb, uint32_t rows, uint32_t cols, const char* filepath)
{
    if (!pgb || rows == 0 || cols == 0) return;
    RB_ESC("\x1b[0m\x1b[39;49m");
    struct doc_stats st;
    compute_doc_stats(pgb, pgb_cursor_pos(pgb), &st);
    uint32_t scroll_offset = compute_scroll_offset(st.cur_line, rows);
    int language = (filepath && filepath[0] != '\0') ? syntax_get_language(filepath) : 0;
    uint32_t gutter = draw_gutter_width();
    uint32_t width = (cols > gutter) ? cols - gutter : 1;
    render_visible_lines(pgb, rows, scroll_offset, width, language);
    position_cursor(st.cur_line, st.cur_col, scroll_offset, cols, rows);
}

static void draw_status(struct global* global, uint32_t cols)
{
    // Modern status bar with better colors and styling
    static const char prefix[] = " zex | ", indent[] = "  ";
    static const char bg_color[] = "\x1b[48;5;238m";      // Darker, more modern background
    static const char fg_color[] = "\x1b[38;5;255m";      // Bright white text
    static const char accent_color[] = "\x1b[38;5;220m";  // Golden yellow for messages
    static const char hint_color[] = "\x1b[38;5;244m";    // Gray for hints
    static const char bold[] = "\x1b[1m";                 // Bold text
    static const char dim[] = "\x1b[2m";                  // Dim text
    static const char hints[] = "Ctrl+S: Save  Ctrl+Q: Quit  Ctrl+F: Search  Ctrl+R: Refresh";

    RB_ESC("\x1b[0m");
    RB_ESC(bg_color);
    RB_ESC(fg_color);

    uint32_t budget = cols;
    rb_append(&rb, prefix, sizeof(prefix) - 1);
    budget -= (budget > (sizeof(prefix) - 1)) ? (uint32_t)(sizeof(prefix) - 1) : budget;

    // File name with bold styling
    uint32_t name_budget = (budget > 10u) ? budget - 10u : 0;
    uint32_t before = name_budget;
    RB_ESC(bold);
    if (global->filepath[0] != '\0') {
        append_sanitized(&rb, global->filepath, strlen(global->filepath), &name_budget);
    } else {
        append_sanitized(&rb, "[No file]", sizeof("[No file]") - 1, &name_budget);
    }
    RB_ESC("\x1b[22m"); // Reset bold
    budget -= before - name_budget;

    // Message with accent color
    if (global->msg.head && budget > 0) {
        char msg_buf[256];
        pgb_to_str(msg_buf, sizeof(msg_buf), &global->msg);
        if (msg_buf[0] != '\0') {
            uint32_t indent_w = (budget > (sizeof(indent) - 1)) ? (uint32_t)(sizeof(indent) - 1) : budget;
            budget -= indent_w;
            rb_append(&rb, indent, indent_w);
            RB_ESC(accent_color);
            RB_ESC(bold);
            uint32_t msg_budget = budget;
            append_sanitized(&rb, msg_buf, strlen(msg_buf), &msg_budget);
            RB_ESC("\x1b[22m"); // Reset bold
            budget -= msg_budget;
        }
    }

    // Hints with dim styling
    if (budget > 0) {
        uint32_t indent_w = (budget > (sizeof(indent) - 1)) ? (uint32_t)(sizeof(indent) - 1) : budget;
        budget -= indent_w;
        rb_append(&rb, indent, indent_w);
        RB_ESC(hint_color);
        RB_ESC(dim);
        uint32_t hints_budget = budget;
        append_sanitized(&rb, hints, sizeof(hints) - 1, &hints_budget);
        RB_ESC("\x1b[22m"); // Reset dim
    }

    RB_ESC("\x1b[0m\x1b[39;49m\x1b[K\r\n");
}

void draw_update(struct global* global)
{
    if (!global) return;
    uint32_t ws_rows = (uint32_t)global->term.ws.ws_row;
    uint32_t ws_cols = (uint32_t)global->term.ws.ws_col;
    if (ws_rows == 0) ws_rows = DRAW_FALLBACK_ROWS;
    if (ws_cols == 0) ws_cols = DRAW_FALLBACK_COLS;
    uint32_t rows_text = (ws_rows >= 2u) ? ws_rows - 1u : 1u; // 1 row for status bar
    rb_clear(&rb);
    draw_cursor_home();
    draw_status(global, ws_cols);
    draw_text(&global->text, rows_text, ws_cols, global->filepath);
    rb_flush(&rb);
}

void draw_deinit(void)
{
    rb_clear(&rb);
    // Modern terminal cleanup
    RB_ESC("\x1b[?25h");   // Show cursor
    RB_ESC("\x1b[0m");     // Reset attributes
    RB_ESC("\x1b[39;49m"); // Default colors
    RB_ESC("\x1b[?1049l"); // Exit alternate screen buffer
    rb_flush(&rb);
    rb_deinit(&rb);
    rb_deinit(&line_disp);
}

uint32_t draw_get_scroll_offset(void)
{
    return last_scroll_offset;
}
