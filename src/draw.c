#include "draw.h"
#include "nodes.h"
#include "render_buffer.h"
#include "syntax.h"
#include "config.h"
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
    if (ts < 1) ts = 1;
    if (ts > 16) ts = 16;
    return (uint32_t)ts;
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

/** Append one byte in display form against a column budget. */
static void append_sanitized_byte(RenderBuffer* out, int byte, uint32_t* budget)
{
    if (!out || !budget || *budget == 0) return;

    unsigned char c = (unsigned char)byte;
    if (c < 32 || c == 127) {
        if (*budget < 2) return; /* caret pair does not fit */
        char pair[2];
        caret_bytes(c, pair);
        if (rb_append(out, pair, 2) != ok) return;
        *budget -= 2;
    } else {
        if (rb_append_char(out, (char)c) != ok) return;
        *budget -= 1;
    }
}

/** Append a byte string in display form, clamped to `*budget` columns. */
static void append_sanitized(RenderBuffer* out, const char* s, size_t len, uint32_t* budget)
{
    if (!s) return;
    for (size_t i = 0; i < len; i++) {
        if (budget && *budget == 0) break;
        append_sanitized_byte(out, (unsigned char)s[i], budget);
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
        /* Never let a huge counter widen the gutter past its fixed width. */
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
    uint32_t line = 0, col = 0;
    uint32_t newlines = 0;
    uint32_t len = 0;
    int ends_with_nl = 1;

    struct pgb_reader it;
    pgb_reader_init(&it, pgb);

    int c;
    while ((c = pgb_reader_next(&it)) >= 0) {
        if (len < cursor_pos) {
            if (c == '\n') {
                line++;
                col = 0;
            } else if (c == '\t') {
                col += tab_size - (col % tab_size);
            } else if (c < 32 || c == 127) {
                col += 2; /* rendered as a two-column caret pair */
            } else {
                col++;
            }
        }

        if (c == '\n') {
            newlines++;
            ends_with_nl = 1;
        } else {
            ends_with_nl = 0;
        }
        len++;
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
 * Store one raw document byte into the display-line scratch buffer,
 * expanding tabs, defusing control characters and enforcing the width cap
 * so the line can never wrap onto the next screen row.
 */
static void line_store_byte(unsigned char c, uint32_t* disp_cols, uint32_t tab_size, uint32_t width)
{
    if (*disp_cols >= width) return; /* rest of the line is clipped */

    if (c == '\t') {
        uint32_t spaces = tab_size - (*disp_cols % tab_size);
        while (spaces-- > 0) {
            if (*disp_cols >= width) break; /* clip a partially fitting tab */
            if (rb_append_char(&line_disp, ' ') != ok) return;
            (*disp_cols)++;
        }
        return;
    }

    if (c < 32 || c == 127) {
        if (width - *disp_cols < 2) return; /* caret pair would overflow */
        char pair[2];
        caret_bytes(c, pair);
        if (rb_append(&line_disp, pair, 2) != ok) return;
        *disp_cols += 2;
        return;
    }

    if (rb_append_char(&line_disp, (char)c) != ok) return;
    (*disp_cols)++;
}

/** Emit the prepared display line: gutter, highlighted body, erase-to-EOL. */
static void emit_visible_line(uint32_t line_num, int language)
{
    draw_line_number(line_num);

    if (language != 0 && line_disp.len > 0) {
        /*
         * NUL-terminate the scratch buffer so the tokeniser can never run
         * off the end, but hand it an explicit length all the same.
         */
        (void)rb_append_char(&line_disp, '\0');
        uint32_t body_len = (uint32_t)line_disp.len - 1u;

        char* hl = (language == 2)
                       ? syntax_highlight_python_line(line_disp.data, body_len)
                       : syntax_highlight_line(line_disp.data, body_len);
        if (hl) {
            rb_append(&rb, hl, strlen(hl));
            free(hl);
        } else {
            /* Highlighter failed (OOM): degrade to plain text, never blank. */
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
    uint32_t rendered = 0;
    uint32_t line_no = 0;
    uint32_t disp_cols = 0;
    int storing = 0;

    rb_clear(&line_disp);

    struct pgb_reader it;
    pgb_reader_init(&it, pgb);

    int c;
    while ((c = pgb_reader_next(&it)) >= 0 && rendered < rows) {
        if (c == '\n') {
            if (storing) {
                emit_visible_line(line_no, language);
                rendered++;
                storing = 0;
            }
            line_no++;
        } else {
            /* Loop guard guarantees rendered < rows here. */
            if (!storing && line_no >= scroll_offset) {
                storing = 1;
                rb_clear(&line_disp);
                disp_cols = 0;
            }
            if (storing) {
                line_store_byte((unsigned char)c, &disp_cols, tab_size, width);
            }
        }
    }

    /* Final line without a trailing newline. */
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
    uint32_t disp_col = (cursor_col < avail) ? cursor_col : avail - 1; /* pin to clipped lines */

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

    int language = 0;
    if (filepath && filepath[0] != '\0') language = syntax_get_language(filepath);

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

    /* Filename: leave a little room for the message zone. */
    {
        uint32_t name_budget = (budget > 10u) ? budget - 10u : 0;
        uint32_t before = name_budget;
        if (global->filepath[0] != '\0') {
            append_sanitized(&rb, global->filepath, strlen(global->filepath), &name_budget);
        } else {
            append_sanitized(&rb, "[No file]", sizeof("[No file]") - 1, &name_budget);
        }
        budget -= before - name_budget;
    }

    /* Flash message in yellow (if any): next priority. */
    struct pgb_reader mit;
    pgb_reader_init(&mit, &global->msg);
    int first = pgb_reader_next(&mit);
    if (first >= 0 && budget > 0) {
        uint32_t indent_w = (budget > (sizeof(indent) - 1)) ? (uint32_t)(sizeof(indent) - 1) : budget;
        budget -= indent_w;
        rb_append(&rb, indent, indent_w);
        RB_ESC(msg_color);

        uint32_t msg_budget = budget;
        append_sanitized_byte(&rb, first, &msg_budget);

        int mc;
        while ((mc = pgb_reader_next(&mit)) >= 0) {
            if (msg_budget == 0) break;
            append_sanitized_byte(&rb, mc, &msg_budget);
        }
        budget -= msg_budget;
    }

    /* Key hints last: they absorb whatever width remains. */
    if (budget > 0) {
        uint32_t indent_w = (budget > (sizeof(indent) - 1)) ? (uint32_t)(sizeof(indent) - 1) : budget;
        budget -= indent_w;
        rb_append(&rb, indent, indent_w);
        RB_ESC(hints_color);

        uint32_t hints_budget = budget;
        append_sanitized(&rb, hints, sizeof(hints) - 1, &hints_budget);
    }

    /* CR-LF explicitly: never rely on the terminal's ONLCR translation. */
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
