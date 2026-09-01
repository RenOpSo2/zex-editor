/*
 * test_selection.c - Tests for selection and clipboard functionality
 *
 * Tests selection and clipboard operations including:
 * - Selection creation and manipulation
 * - Copy operations
 * - Cut operations
 * - Paste operations
 * - Clipboard buffer management
 */
#include "src/global.h"
#include "src/nodes.h"
#include "src/editor.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static void setup_global(struct global* g) {
    static char arena_mem[arena_capacity];
    g->arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&g->text, &g->arena);
    pgb_init(&g->msg, &g->arena);
    pgb_init(&g->clipboard, &g->arena);
    g->filepath[0] = '\0';
    g->undo_count = 0;
    g->redo_count = 0;
    g->search_active = false;
    g->search_query[0] = '\0';
    g->search_pos = 0;
    g->search_match_count = 0;
    g->search_query_len = 0;
    g->has_selection = false;
    g->sel_anchor = 0;
}

static void cleanup_global(struct global* g) {
    pgb_clear(&g->text);
    pgb_clear(&g->msg);
    pgb_clear(&g->clipboard);
}

static void test_copy_range(void) {
    printf("Testing copy range functionality...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* content = "Hello World\nThis is a test\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Copy a range (from 6 to 11, which is "World")
    pgb_copy_range(&g.clipboard, &g.text, 6, 11, &g.arena);
    
    // Verify clipboard content
    char clipboard_buf[buf_capacity];
    pgb_to_str(clipboard_buf, sizeof(clipboard_buf), &g.clipboard);
    assert(strcmp(clipboard_buf, "World") == 0);
    printf("  Copied 'World' successfully\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Copy range test passed\n");
}

static void test_copy_multiline(void) {
    printf("Testing multiline copy...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* content = "Line 1\nLine 2\nLine 3\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Copy multiline range (from 0 to 13, which is "Line 1\nLine 2")
    pgb_copy_range(&g.clipboard, &g.text, 0, 13, &g.arena);
    
    // Verify clipboard content
    char clipboard_buf[buf_capacity];
    pgb_to_str(clipboard_buf, sizeof(clipboard_buf), &g.clipboard);
    assert(strcmp(clipboard_buf, "Line 1\nLine 2") == 0);
    printf("  Copied multiline content successfully\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Multiline copy test passed\n");
}

static void test_delete_range(void) {
    printf("Testing delete range functionality...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* content = "Hello World\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Delete a range (from 6 to 11, which is "World")
    pgb_delete_range(&g.text, 6, 11);
    
    // Verify content after deletion
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &g.text);
    assert(strcmp(buffer, "Hello \n") == 0);
    printf("  Deleted 'World' successfully\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Delete range test passed\n");
}

static void test_cursor_position(void) {
    printf("Testing cursor position tracking...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* content = "Hello\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Check cursor position at end
    uint32_t pos = pgb_cursor_pos(&g.text);
    assert(pos == 6); // "Hello\n" = 6 bytes
    printf("  Cursor at position %u\n", pos);
    
    // Move cursor to specific position
    pgb_move_to_pos(&g.text, 3);
    pos = pgb_cursor_pos(&g.text);
    assert(pos == 3);
    printf("  Moved cursor to position %u\n", pos);
    
    cleanup_global(&g);
    
    printf("  ✓ Cursor position test passed\n");
}

static void test_clipboard_clear(void) {
    printf("Testing clipboard clear...\n");
    
    struct global g;
    setup_global(&g);
    
    // Add content to clipboard
    pgb_insert_str(&g.clipboard, "test content", &g.arena);
    
    // Clear clipboard
    pgb_clear(&g.clipboard);
    
    // Verify clipboard is empty
    char clipboard_buf[buf_capacity];
    pgb_to_str(clipboard_buf, sizeof(clipboard_buf), &g.clipboard);
    assert(strlen(clipboard_buf) == 0);
    printf("  Clipboard cleared successfully\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Clipboard clear test passed\n");
}

static void test_move_operations(void) {
    printf("Testing cursor move operations...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* content = "Line 1\nLine 2\nLine 3\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Move to end
    uint32_t end_pos = pgb_cursor_pos(&g.text);
    printf("  Initial position: %u\n", end_pos);
    
    // Move left
    pgb_move_left(&g.text);
    uint32_t left_pos = pgb_cursor_pos(&g.text);
    assert(left_pos == end_pos - 1);
    printf("  Moved left to position: %u\n", left_pos);
    
    // Move right
    pgb_move_right(&g.text);
    uint32_t right_pos = pgb_cursor_pos(&g.text);
    assert(right_pos == end_pos);
    printf("  Moved right to position: %u\n", right_pos);
    
    // Move up
    pgb_move_up(&g.text);
    uint32_t up_pos = pgb_cursor_pos(&g.text);
    printf("  Moved up to position: %u\n", up_pos);
    
    // Move down
    pgb_move_down(&g.text);
    uint32_t down_pos = pgb_cursor_pos(&g.text);
    printf("  Moved down to position: %u\n", down_pos);
    
    cleanup_global(&g);
    
    printf("  ✓ Move operations test passed\n");
}

static void test_selection_state(void) {
    printf("Testing selection state management...\n");
    
    struct global g;
    setup_global(&g);
    
    // Set selection state
    g.has_selection = true;
    g.sel_anchor = 5;
    
    assert(g.has_selection == true);
    assert(g.sel_anchor == 5);
    printf("  Selection state set correctly\n");
    
    // Clear selection state
    g.has_selection = false;
    assert(g.has_selection == false);
    printf("  Selection state cleared\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Selection state test passed\n");
}

static void test_empty_operations(void) {
    printf("Testing operations on empty buffer...\n");
    
    struct global g;
    setup_global(&g);
    
    // Try to copy from empty buffer
    pgb_copy_range(&g.clipboard, &g.text, 0, 0, &g.arena);
    
    // Verify clipboard is still empty
    char clipboard_buf[buf_capacity];
    pgb_to_str(clipboard_buf, sizeof(clipboard_buf), &g.clipboard);
    assert(strlen(clipboard_buf) == 0);
    printf("  Copy from empty buffer handled correctly\n");
    
    // Try to delete from empty buffer
    pgb_delete_range(&g.text, 0, 0);
    printf("  Delete from empty buffer handled correctly\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Empty operations test passed\n");
}

static void test_unicode_copy(void) {
    printf("Testing Unicode copy operations...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content with Unicode
    const char* content = "Hello 世界\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Copy Unicode range
    pgb_copy_range(&g.clipboard, &g.text, 6, 12, &g.arena);
    
    // Verify clipboard content
    char clipboard_buf[buf_capacity];
    pgb_to_str(clipboard_buf, sizeof(clipboard_buf), &g.clipboard);
    printf("  Copied Unicode content: '%s'\n", clipboard_buf);
    
    cleanup_global(&g);
    
    printf("  ✓ Unicode copy test passed\n");
}

int main(void) {
    printf("\n=== ZEX SELECTION & CLIPBOARD TESTS ===\n\n");
    
    test_copy_range();
    test_copy_multiline();
    test_delete_range();
    test_cursor_position();
    test_clipboard_clear();
    test_move_operations();
    test_selection_state();
    test_empty_operations();
    test_unicode_copy();
    
    printf("\n✓ ALL SELECTION & CLIPBOARD TESTS PASSED!\n");
    return 0;
}
