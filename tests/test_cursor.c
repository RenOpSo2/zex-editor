/*
 * test_cursor.c - Regression tests for cursor accuracy.
 *
 * These guard the fix for the bug where the cursor was positioned by counting
 * one display column per byte, which silently drifted on any non-ASCII text
 * (documentation, code comments, CJK, emoji, ...). The shared display-width
 * abstraction in src/dispwidth.c is the single source of truth.
 */
#include "src/editor.h"
#include "src/nodes.h"
#include "src/global.h"
#include "src/dispwidth.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void setup_global(struct global* g) {
    static char arena_mem[arena_capacity];
    g->arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&g->text, &g->arena);
    pgb_init(&g->msg, &g->arena);
    g->filepath[0] = '\0';
    g->undo_count = 0;
    g->redo_count = 0;
    g->search_active = false;
    g->search_query[0] = '\0';
    g->search_pos = 0;
    g->search_match_count = 0;
}

/* --- The display-width abstraction is the root-cause fix. --- */
static void test_dispwidth(void) {
    printf("Testing display-width abstraction...\n");

    /* ASCII: one byte, one column. */
    unsigned char a = 'a';
    size_t consumed;
    assert(char_width(&a, 1, 0, 4, &consumed) == 1);
    assert(consumed == 1);

    /* Tab: expands against the current column. */
    assert(char_width(&a, 1, 1, 4, &consumed) == 1); /* 'a' again */
    unsigned char t = '\t';
    assert(char_width(&t, 1, 0, 4, &consumed) == 4); /* col 0 -> 4 */
    assert(char_width(&t, 1, 1, 4, &consumed) == 3); /* col 1 -> 4 */

    /* Latin-1 supplement: 'é' = C3 A9, two bytes but ONE column. */
    unsigned char e_acute[] = {0xC3, 0xA9};
    assert(char_width(e_acute, 2, 0, 4, &consumed) == 1);
    assert(consumed == 2);

    /* CJK: '中' = E4 B8 AD, three bytes, TWO columns. */
    unsigned char zhong[] = {0xE4, 0xB8, 0xAD};
    assert(char_width(zhong, 3, 0, 4, &consumed) == 2);
    assert(consumed == 3);

    /* Emoji: '😀' = F0 9F 98 80, four bytes, TWO columns. */
    unsigned char smile[] = {0xF0, 0x9F, 0x98, 0x80};
    assert(char_width(smile, 4, 0, 4, &consumed) == 2);
    assert(consumed == 4);

    /* Malformed lead: falls back to one raw byte, never desyncs. */
    unsigned char bad[] = {0xC3};
    assert(char_width(bad, 1, 0, 4, &consumed) == 1);
    assert(consumed == 1);

    printf("  \xe2\x9c\x93 display-width counts bytes and columns correctly\n");
}

/* A helper that places the cursor at the very end of the buffer. */
static void insert_end(struct global* g, const char* s) {
    pgb_insert_str(&g->text, s, &g->arena);
}

/* Vertical movement must preserve the display column across a line that
 * contains a wide (multi-byte) character. */
static void test_move_up_unicode(void) {
    printf("Testing vertical move over Unicode line...\n");

    struct global g;
    setup_global(&g);

    /* line1 = "a中b" (display cols: 1 + 2 + 1 = 4), line2 = "xy". */
    insert_end(&g, "a\xe4\xb8\xad""b\nxy");
    assert(pgb_cursor_pos(&g.text) == 8); /* a(1) + 中(3) + b(1) + \n(1) + x(1) + y(1) */

    pgb_move_up(&g.text);

    /* Cursor is on line1 at display column 2. The only valid position there
     * is right after 'a' (byte offset 1); the old byte-counting code would
     * have landed inside the middle of '中' (offset 2). */
    assert(pgb_cursor_pos(&g.text) == 1);

    printf("  \xe2\x9c\x93 pgb_move_up preserves horizontal column on Unicode\n");
}

static void test_move_up_tab(void) {
    printf("Testing vertical move over a tabbed line...\n");

    struct global g;
    setup_global(&g);

    /* line1 = "a\tb" (a, tab -> col 4, b at col 4..5), line2 = "xy". */
    insert_end(&g, "a\tb\nxy");
    assert(pgb_cursor_pos(&g.text) == 6);

    pgb_move_up(&g.text);

    /* Target display column is 2, which falls inside the expanded tab; the
     * cursor must sit right after 'a' (byte offset 1), not after the tab. */
    assert(pgb_cursor_pos(&g.text) == 1);

    printf("  \xe2\x9c\x93 pgb_move_up honours tab expansion\n");
}

static void test_move_down_unicode(void) {
    printf("Testing vertical move down over Unicode line...\n");

    struct global g;
    setup_global(&g);

    insert_end(&g, "a\xe4\xb8\xad""b\nxy"); /* cursor at end of line2 */

    /* Go to the start of the buffer, then one character in (display col 1). */
    pgb_move_to_pos(&g.text, 0);
    pgb_move_right(&g.text); /* after 'a', display col 1 */
    assert(pgb_cursor_pos(&g.text) == 1);

    pgb_move_down(&g.text); /* should land on line2 at display col 1 (after 'x') */

    /* line1 length in bytes: a(1)+中(3)+b(1)+\n(1) = 6; 'x' is at 6, after 'x' is 7. */
    assert(pgb_cursor_pos(&g.text) == 7);

    printf("  \xe2\x9c\x93 pgb_move_down preserves horizontal column on Unicode\n");
}

int main(void) {
    printf("\n=== ZEX CURSOR ACCURACY TESTS ===\n\n");
    test_dispwidth();
    test_move_up_unicode();
    test_move_up_tab();
    test_move_down_unicode();
    printf("\n\xe2\x9c\x93 ALL CURSOR TESTS PASSED!\n");
    return 0;
}
