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

/*
 * Rendering pipeline
 *
 * Each frame is assembled in memory (RenderBuffer) and written out with a
 * single robust flush, so the terminal never sees a half-written escape
 * sequence. The document is streamed straight out of the paged gap buffer
 * through pgb_reader: there is no intermediate flattened copy, so documents
 * larger than memory-bounded scratch space still render correctly and NUL
 * bytes cannot confuse the walk (everything is length-driven).
 *
 * Untrusted bytes are neutralised before they reach the terminal:
 *  - C0 control characters and DEL render in caret notation (^A, ^?, ...)
 *    instead of being interpreted (which would corrupt or inject output),
 *  - tabs expand to the configured tab stop,
 *  - every logical line is clipped to the available width so a long line
 *    can never wrap and shift the whole layout down.
 */

/* Frame layout: row 1 = status bar, rows 2..N-1 = text, last row spare. */
#define DRAW_FALLBACK_ROWS 24u
#define DRAW_FALLBACK_COLS 80u

static RenderBuffer rb;         /* frame assembly buffer */
static RenderBuffer line_disp;  /* scratch: current line after tab expansion,
                                 * sanitisation and width clipping */
static uint32_t last_scroll_offset = 0;

#define RB_ESC(s) ((void)rb_append(&rb, (s), sizeof(s) - 1))

/** Configured tab size, clamped to a safe range (never zero: modulo guard). */
static uint32_t draw_tab_size(void)
{
    int ts = (int)config_get_number("tabsize", 4);
    return (uint32_t)(ts < 1 ? 1 : ts > 16 ? 16 : ts);
}

/**
 * Columns occupied by the line-number gutter.
 * Single source of truth shared with mouse hit-testing in input.c.
 * "%5u " => five digits plus one separating space.
 */
uint32_t draw_gutter_width(void)
{
    return config_get_bool("show_line_numbers", 1) ? 6u : 0u;
}

/** Render one control character in caret notation. Returns its width. */
static uint32_t caret_bytes(unsigned char c, char out[2])
{
    out[0] = '^';
    out[1] = (c == 127) ? '?' : (char)(c + '@');
    return 2;
}

/** Append a byte string in display form, clamped to `*budget` columns.
 *  Honours the same width rules as the document renderer (UTF-8 aware). */
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
            char pair[2];
            caret_bytes(b, pair);
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

/** Draw the gutter line-number cell for a visible line. */
static void draw_line_number(uint32_t line_num)
{
    if (!config_get_bool("show_line_numbers", 1)) return;

    RB_ESC("\x1b[0m\x1b[38;5;244m");
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

/**
 * Single pass over the document computing everything the frame needs:
 * the cursor's line/column in display coordinates and the total number of
 * logical lines. Length-driven, so embedded NUL bytes are harmless.
 */
struct doc_stats {
    uint32_t cur_line;
    uint32_t cur_col;
    uint32_t total_lines;
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

/**
 * Store one document character (the raw bytes in `ch`, length `n`) into the
 * display-line scratch buffer, expanding tabs, defusing control characters and
 * enforcing the width cap so the line can never wrap onto the next screen row.
 *
 * The whole character is written as a single unit: a multi-byte UTF-8
 * sequence is never split across the clip boundary, which keeps the rendered
 * text and the cursor column (computed by the same width rules) in lock-step.
 */
static void line_store_char(const unsigned char* ch, size_t n, uint32_t* disp_cols,
                            uint32_t tab_size, uint32_t width)
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
        char pair[2];
        caret_bytes(b, pair);
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

/** Emit the prepared display line: gutter, highlighted body, erase-to-EOL. */
static void emit_visible_line(uint32_t line_num, int language)
{
    draw_line_number(line_num);

    if (language != 0 && line_disp.len > 0) {
        (void)rb_append_char(&line_disp, '\0');
        uint32_t body_len = (uint32_t)line_disp.len - 1u;

        char* hl = (language == 2)
                       ? syntax_highlight_python_line(line_disp.data, body_len)
                       : syntax_highlight_line(line_disp.data, body_len);
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

/**
 * Stream the visible window [scroll_offset, scroll_offset + rows) out of
 * the document and pad the remaining rows so the frame height is exact.
 */
static void render_visible_lines(const struct paged_gap_buffer* pgb, uint32_t rows, uint32_t scroll_offset,
                                 uint32_t width, int language)
{
    uint32_t tab_size = draw_tab_size();
    uint32_t rendered = 0, line_no = 0, disp_cols = 0;
    int storing = 0;

    rb_clear(&line_disp);

    struct pgb_reader it;
    pgb_reader_init(&it, pgb);

    int c;
    unsigned char ch[4];
    while ((c = pgb_reader_next(&it)) >= 0 && rendered < rows) {
        if (c == '\n') {
            if (storing) {
                emit_visible_line(line_no, language);
                rendered++;
                storing = 0;
            }
            line_no++;
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

        if (!storing && line_no >= scroll_offset) {
            storing = 1;
            rb_clear(&line_disp);
            disp_cols = 0;
        }
        if (storing) {
            line_store_char(ch, got, &disp_cols, tab_size, width);
        }
    }

    if (storing) {
        emit_visible_line(line_no, language);
        rendered++;
    }

    for (; rendered < rows; rendered++) {
        RB_ESC("\x1b[K\r\n");
    }
}

static void position_cursor(uint32_t cursor_line, uint32_t cursor_col, uint32_t scroll_offset, uint32_t cols,
                            uint32_t rows)
{
    uint32_t vis_line = (cursor_line >= scroll_offset) ? cursor_line - scroll_offset : 0;
    if (rows > 0 && vis_line > rows - 1) vis_line = rows - 1;

    uint32_t gutter = draw_gutter_width();
    uint32_t avail = (cols > gutter) ? cols - gutter : 1;
    uint32_t disp_col = (cursor_col < avail) ? cursor_col : avail - 1;

    char seq[32];
    int n = snprintf(seq, sizeof(seq), "\x1b[%u;%uH", (unsigned)(vis_line + 2u), (unsigned)(disp_col + gutter + 1u));
    if (n > 0 && (size_t)n < sizeof(seq)) {
        rb_append(&rb, seq, (size_t)n);
    }
    RB_ESC("\x1b[?25h");
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

/**
 * Status bar. Filename and flash message are sanitised (a hostile path or
 * message cannot inject escapes) and clamped to the real terminal width.
 * Priority on narrow terminals: path, then flash message, then hints —
 * transient feedback must never be starved by the static help text.
 */
static void draw_status(struct global* global, uint32_t cols)
{
    static const char prefix[] = " zex | ";
    static const char indent[] = "  ";
    static const char hints_color[] = "\x1b[38;5;244m";
    static const char hints[] = "Ctrl+S: Save  Ctrl+Q: Quit  Ctrl+F: Search  Ctrl+R: Refresh";
    static const char msg_color[] = "\x1b[38;5;220m";

    RB_ESC("\x1b[0m\x1b[48;5;235m\x1b[38;5;250m");

    uint32_t budget = cols;
    rb_append(&rb, prefix, sizeof(prefix) - 1);
    budget -= (budget > (sizeof(prefix) - 1)) ? (uint32_t)(sizeof(prefix) - 1) : budget;

    uint32_t name_budget = (budget > 10u) ? budget - 10u : 0;
    uint32_t before = name_budget;
    if (global->filepath[0] != '\0') {
        append_sanitized(&rb, global->filepath, strlen(global->filepath), &name_budget);
    } else {
        append_sanitized(&rb, "[No file]", sizeof("[No file]") - 1, &name_budget);
    }
    budget -= before - name_budget;

    if (global->msg.head && budget > 0) {
        char msg_buf[256];
        pgb_to_str(msg_buf, sizeof(msg_buf), &global->msg);
        if (msg_buf[0] != '\0') {
            uint32_t indent_w = (budget > (sizeof(indent) - 1)) ? (uint32_t)(sizeof(indent) - 1) : budget;
            budget -= indent_w;
            rb_append(&rb, indent, indent_w);
            RB_ESC(msg_color);

            uint32_t msg_budget = budget;
            append_sanitized(&rb, msg_buf, strlen(msg_buf), &msg_budget);
            budget -= msg_budget;
        }
    }

    if (budget > 0) {
        uint32_t indent_w = (budget > (sizeof(indent) - 1)) ? (uint32_t)(sizeof(indent) - 1) : budget;
        budget -= indent_w;
        rb_append(&rb, indent, indent_w);
        RB_ESC(hints_color);

        uint32_t hints_budget = budget;
        append_sanitized(&rb, hints, sizeof(hints) - 1, &hints_budget);
    }

    RB_ESC("\x1b[0m\x1b[39;49m\x1b[K\r\n");
}

void draw_update(struct global* global)
{
    if (!global) return;

    /* Guard degenerate sizes: ioctl may have failed or reported zeros. */
    uint32_t ws_rows = (uint32_t)global->term.ws.ws_row;
    uint32_t ws_cols = (uint32_t)global->term.ws.ws_col;
    if (ws_rows == 0) ws_rows = DRAW_FALLBACK_ROWS;
    if (ws_cols == 0) ws_cols = DRAW_FALLBACK_COLS;
    uint32_t rows_text = (ws_rows >= 3u) ? ws_rows - 2u : 1u;

    rb_clear(&rb);

    draw_cursor_home();
    draw_status(global, ws_cols);
    draw_text(&global->text, rows_text, ws_cols, global->filepath);

    rb_flush(&rb);
}

void draw_deinit(void)
{
    rb_clear(&rb);

    RB_ESC("\x1b[?25h");
    RB_ESC("\x1b[0m\x1b[39;49m");
    RB_ESC("\x1b[?1049l");

    rb_flush(&rb);

    rb_deinit(&rb);
    rb_deinit(&line_disp);
}

uint32_t draw_get_scroll_offset(void)
{
    return last_scroll_offset;
}
