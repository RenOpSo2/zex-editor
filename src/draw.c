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
#define UTF8_MAX_BYTES 4
#define LINE_NUM_BUF_SIZE 16
#define STATUS_MSG_BUF_SIZE 256
#define CURSOR_SEQ_BUF_SIZE 32
#define ANSI_RESET "\x1b[0m"
#define ANSI_DEFAULT_FG "\x1b[39m"
#define ANSI_DEFAULT_BG "\x1b[49m"
#define ANSI_DEFAULT_COLORS ANSI_DEFAULT_FG ANSI_DEFAULT_BG
#define ANSI_BOLD "\x1b[1m"
#define ANSI_DIM "\x1b[2m"
#define ANSI_BOLD_OFF "\x1b[22m"
#define ANSI_DIM_OFF "\x1b[22m"
#define ANSI_CLEAR_LINE "\x1b[K"
#define ANSI_CLEAR_SCREEN "\x1b[2J"
#define ANSI_CURSOR_HOME "\x1b[H"
#define ANSI_CURSOR_SHOW "\x1b[?25h"
#define ANSI_CURSOR_HIDE "\x1b[?25l"
#define ANSI_ALT_SCREEN_ENABLE "\x1b[?1049h"
#define ANSI_ALT_SCREEN_DISABLE "\x1b[?1049l"
#define ANSI_LN_COLOR "\x1b[38;5;238m"
#define ANSI_STATUS_BG "\x1b[48;5;238m"
#define ANSI_STATUS_FG "\x1b[38;5;255m"
#define ANSI_ACCENT_COLOR "\x1b[38;5;220m"
#define ANSI_HINT_COLOR "\x1b[38;5;244m"

static RenderBuffer rb, line_disp;
static uint32_t last_scroll_offset = 0;

#define RB_ESC(s) ((void)rb_append(&rb, (s), sizeof(s) - 1))

static uint32_t draw_tab_size(void)
{
    int ts = (int)config_get_number("tabsize", 4);
    if (ts < 1) ts = 1;
    if (ts > 16) ts = 16;
    return (uint32_t)ts;
}

uint32_t draw_gutter_width(void)
{
    return config_get_bool("show_line_numbers", 1) ? 6u : 0u;
}

static void append_sanitized(RenderBuffer* out, const char* s, size_t len, uint32_t* budget)
{
    if (!out || !s || !budget || len == 0) return;
    
    size_t i = 0;
    while (i < len && *budget > 0) {
        unsigned char ch[UTF8_MAX_BYTES];
        size_t got = 0;
        
        ch[got++] = (unsigned char)s[i++];
        int extra = utf8_trail_count(ch[0]);
        
        for (int k = 0; k < extra && i < len && got < UTF8_MAX_BYTES; k++) {
            ch[got++] = (unsigned char)s[i++];
        }
        
        unsigned char b = ch[0];
        if (b < 32 || b == 127) {
            if (*budget < 2) break;
            char pair[2] = {'^', (char)(b == 127 ? '?' : (char)(b + '@'))};
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
    
    RB_ESC(ANSI_RESET ANSI_LN_COLOR);
    RB_ESC(ANSI_BOLD);
    
    char buf[LINE_NUM_BUF_SIZE];
    int n = snprintf(buf, sizeof(buf), "%5u ", (unsigned)(line_num + 1u));
    if (n > 0 && n < (int)sizeof(buf)) {
        uint32_t gutter = draw_gutter_width();
        size_t out = (size_t)n;
        if (out > gutter) out = gutter;
        rb_append(&rb, buf, out);
    }
    
    RB_ESC(ANSI_RESET ANSI_DEFAULT_COLORS);
}

void draw_init(void)
{
    rb_init(&rb);
    rb_init(&line_disp);
    
    RB_ESC(ANSI_ALT_SCREEN_ENABLE);
    RB_ESC(ANSI_RESET);
    RB_ESC(ANSI_DEFAULT_COLORS);
    RB_ESC(ANSI_CLEAR_SCREEN);
    RB_ESC(ANSI_CURSOR_HOME);
    RB_ESC(ANSI_CURSOR_HIDE);
    rb_flush(&rb);
}

static void draw_cursor_home(void)
{
    RB_ESC(ANSI_RESET);
    RB_ESC(ANSI_DEFAULT_COLORS);
    RB_ESC(ANSI_CURSOR_HOME);
    RB_ESC(ANSI_CLEAR_SCREEN);
}

struct doc_stats {
    uint32_t cur_line, cur_col, total_lines;
};

static void compute_doc_stats(const struct paged_gap_buffer* pgb, uint32_t cursor_pos, struct doc_stats* st)
{
    if (!pgb || !st) {
        if (st) {
            st->cur_line = 0;
            st->cur_col = 0;
            st->total_lines = 0;
        }
        return;
    }
    
    uint32_t tab_size = draw_tab_size();
    uint32_t line = 0, col = 0, newlines = 0, len = 0;
    int ends_with_nl = 1;
    struct pgb_reader it;
    pgb_reader_init(&it, pgb);
    
    int c;
    while ((c = pgb_reader_next(&it)) >= 0) {
        unsigned char ch[UTF8_MAX_BYTES];
        size_t got = 0;
        
        ch[got++] = (unsigned char)c;
        int extra = utf8_trail_count((unsigned char)c);
        
        for (int k = 0; k < extra && got < UTF8_MAX_BYTES; k++) {
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
    if (size_y == 0) return 0;
    
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
    if (!ch || !disp_cols || *disp_cols >= width || tab_size == 0) return;
    
    unsigned char b = ch[0];
    if (b == '\t') {
        uint32_t spaces = tab_size - (*disp_cols % tab_size);
        while (spaces-- > 0 && *disp_cols < width) {
            if (rb_append_char(&line_disp, ' ') != ok) return;
            (*disp_cols)++;
        }
        return;
    }
    
    if (b < 32 || b == 127) {
        if (width - *disp_cols < 2) return;
        char pair[2] = {'^', (char)(b == 127 ? '?' : (char)(b + '@'))};
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
        if (rb_append_char(&line_disp, '\0') == ok) {
            uint32_t body_len = (uint32_t)line_disp.len - 1u;
            char* hl = (language == 2) ? syntax_highlight_python_line(line_disp.data, body_len) : syntax_highlight_line(line_disp.data, body_len);
            if (hl) {
                rb_append(&rb, hl, strlen(hl));
                free(hl);
            } else {
                rb_append(&rb, line_disp.data, body_len);
            }
        }
    } else {
        rb_append(&rb, line_disp.data, line_disp.len);
    }
    
    RB_ESC(ANSI_CLEAR_LINE "\r\n");
}

static void render_visible_lines(const struct paged_gap_buffer* pgb, uint32_t rows, uint32_t scroll_offset, uint32_t width, int language)
{
    if (!pgb || rows == 0 || width == 0) return;
    
    uint32_t tab_size = draw_tab_size();
    uint32_t rendered = 0, line_no = 0, disp_cols = 0;
    rb_clear(&line_disp);
    struct pgb_reader it;
    pgb_reader_init(&it, pgb);
    
    int c;
    while (line_no < scroll_offset && (c = pgb_reader_next(&it)) >= 0) {
        if (c == '\n') {
            line_no++;
        }
    }
    
    while ((c = pgb_reader_next(&it)) >= 0 && rendered < rows) {
        if (c == '\n') {
            emit_visible_line(line_no, language);
            rendered++;
            line_no++;
            rb_clear(&line_disp);
            disp_cols = 0;
            continue;
        }
        
        unsigned char ch[UTF8_MAX_BYTES];
        size_t got = 0;
        ch[got++] = (unsigned char)c;
        int extra = utf8_trail_count((unsigned char)c);
        
        for (int k = 0; k < extra && got < UTF8_MAX_BYTES; k++) {
            int d = pgb_reader_next(&it);
            if (d < 0) break;
            ch[got++] = (unsigned char)d;
        }
        
        line_store_char(ch, got, &disp_cols, tab_size, width);
    }
    
    if (line_disp.len > 0) {
        emit_visible_line(line_no, language);
        rendered++;
    }
    
    for (; rendered < rows; rendered++) {
        RB_ESC(ANSI_CLEAR_LINE "\r\n");
    }
}

static void position_cursor(uint32_t cursor_line, uint32_t cursor_col, uint32_t scroll_offset, uint32_t cols, uint32_t rows)
{
    if (cols == 0 || rows == 0) return;
    
    uint32_t vis_line = (cursor_line >= scroll_offset) ? cursor_line - scroll_offset : 0;
    if (vis_line > rows - 1) vis_line = rows - 1;
    
    uint32_t gutter = draw_gutter_width();
    uint32_t avail = (cols > gutter) ? cols - gutter : 1;
    uint32_t disp_col = (cursor_col < avail) ? cursor_col : avail - 1;
    
    char seq[CURSOR_SEQ_BUF_SIZE];
    int n = snprintf(seq, sizeof(seq), "\x1b[%u;%uH", (unsigned)(vis_line + 1u), (unsigned)(disp_col + gutter + 1u));
    if (n > 0 && (size_t)n < sizeof(seq)) {
        rb_append(&rb, seq, (size_t)n);
    }
    
    RB_ESC(ANSI_CURSOR_SHOW);
}

static void draw_text(const struct paged_gap_buffer* pgb, uint32_t rows, uint32_t cols, const char* filepath, struct doc_stats* st)
{
    if (!pgb || rows == 0 || cols == 0 || !st) return;
    
    RB_ESC(ANSI_RESET ANSI_DEFAULT_COLORS);
    
    uint32_t scroll_offset = compute_scroll_offset(st->cur_line, rows);
    int language = (filepath && filepath[0] != '\0') ? syntax_get_language(filepath) : 0;
    uint32_t gutter = draw_gutter_width();
    uint32_t width = (cols > gutter) ? cols - gutter : 1;
    
    render_visible_lines(pgb, rows, scroll_offset, width, language);
}

static void draw_status(struct global* global, uint32_t cols)
{
    if (!global || cols == 0) return;
    
    static const char prefix[] = " zex | ";
    static const char indent[] = "  ";
    static const char hints[] = "Ctrl+S: Save  Ctrl+Q: Quit  Ctrl+F: Search  Ctrl+R: Refresh";
    static const char no_file[] = "[No file]";

    RB_ESC(ANSI_RESET);
    RB_ESC(ANSI_STATUS_BG);
    RB_ESC(ANSI_STATUS_FG);

    uint32_t budget = cols;
    size_t prefix_len = sizeof(prefix) - 1;
    if (budget > prefix_len) {
        rb_append(&rb, prefix, prefix_len);
        budget -= (uint32_t)prefix_len;
    } else {
        budget = 0;
    }

    if (budget > 10u) {
        uint32_t name_budget = budget - 10u;
        uint32_t before = name_budget;
        RB_ESC(ANSI_BOLD);
        
        if (global->filepath[0] != '\0') {
            append_sanitized(&rb, global->filepath, strlen(global->filepath), &name_budget);
        } else {
            append_sanitized(&rb, no_file, sizeof(no_file) - 1, &name_budget);
        }
        
        RB_ESC(ANSI_BOLD_OFF);
        budget -= (before - name_budget);
    }

    if (global->msg.head && budget > 0) {
        char msg_buf[STATUS_MSG_BUF_SIZE];
        pgb_to_str(msg_buf, sizeof(msg_buf), &global->msg);
        if (msg_buf[0] != '\0') {
            size_t indent_len = sizeof(indent) - 1;
            if (budget > indent_len) {
                rb_append(&rb, indent, indent_len);
                budget -= (uint32_t)indent_len;
                RB_ESC(ANSI_ACCENT_COLOR);
                RB_ESC(ANSI_BOLD);
                uint32_t msg_budget = budget;
                append_sanitized(&rb, msg_buf, strlen(msg_buf), &msg_budget);
                RB_ESC(ANSI_BOLD_OFF);
                budget -= msg_budget;
            }
        }
    }

    if (budget > 0) {
        size_t indent_len = sizeof(indent) - 1;
        if (budget > indent_len) {
            rb_append(&rb, indent, indent_len);
            budget -= (uint32_t)indent_len;
            RB_ESC(ANSI_HINT_COLOR);
            RB_ESC(ANSI_DIM);
            uint32_t hints_budget = budget;
            append_sanitized(&rb, hints, sizeof(hints) - 1, &hints_budget);
            RB_ESC(ANSI_DIM_OFF);
        }
    }

    RB_ESC(ANSI_RESET ANSI_DEFAULT_COLORS ANSI_CLEAR_LINE "\r\n");
}

void draw_update(struct global* global)
{
    if (!global) return;
    
    uint32_t ws_rows = (uint32_t)global->term.ws.ws_row;
    uint32_t ws_cols = (uint32_t)global->term.ws.ws_col;
    if (ws_rows == 0) ws_rows = DRAW_FALLBACK_ROWS;
    if (ws_cols == 0) ws_cols = DRAW_FALLBACK_COLS;
    uint32_t rows_text = (ws_rows >= 2u) ? ws_rows - 1u : 1u;
    
    rb_clear(&rb);
    draw_cursor_home();
    draw_status(global, ws_cols);
    
    struct doc_stats st;
    compute_doc_stats(&global->text, pgb_cursor_pos(&global->text), &st);
    draw_text(&global->text, rows_text, ws_cols, global->filepath, &st);
    
    position_cursor(st.cur_line, st.cur_col, compute_scroll_offset(st.cur_line, rows_text), ws_cols, rows_text);
    rb_flush(&rb);
}

void draw_deinit(void)
{
    rb_clear(&rb);
    RB_ESC(ANSI_CURSOR_SHOW);
    RB_ESC(ANSI_RESET);
    RB_ESC(ANSI_DEFAULT_COLORS);
    RB_ESC(ANSI_ALT_SCREEN_DISABLE);
    rb_flush(&rb);
    rb_deinit(&rb);
    rb_deinit(&line_disp);
}

uint32_t draw_get_scroll_offset(void)
{
    return last_scroll_offset;
}
