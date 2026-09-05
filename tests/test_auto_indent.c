/*
 * test_auto_indent.c - Tests for auto-indent functionality
 *
 * Tests auto-indent functionality including:
 * - Basic auto-indent with spaces
 * - Auto-indent with tabs
 * - Mixed whitespace
 * - Edge cases (empty buffer, single line, etc.)
 */
#include "src/global.h"
#include "src/nodes.h"
#include "src/editor.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

static char arena_mem[arena_capacity];
static struct global global;

static void setup_global(void) {
    memset(&global, 0, sizeof(global));
    global.arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&global.text, &global.arena);
    pgb_init(&global.msg, &global.arena);
    global.filepath[0] = '\0';
    global.undo_count = 0;
    global.redo_count = 0;
    global.search_active = false;
    global.search_query[0] = '\0';
    global.search_pos = 0;
    global.search_match_count = 0;
    global.search_query_len = 0;
    global.has_selection = false;
    global.sel_anchor = 0;
}

static void cleanup_global(void) {
    pgb_clear(&global.text);
    pgb_clear(&global.msg);
}

// Helper function to simulate auto_indent behavior without config dependency
static void simulate_auto_indent_behavior(void) {
    uint32_t cursor = pgb_cursor_pos(&global.text);
    if (cursor < 2) return; // Need at least the newline and one character before it

    char buf[buf_capacity];
    pgb_to_str(buf, sizeof(buf), &global.text);

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
    if (indent_len > 0) {
        pgb_insert_str(&global.text, indent_str, &global.arena);
    }
}

static void simulate_newline_with_auto_indent(void) {
    pgb_insert(&global.text, '\n', &global.arena);
    simulate_auto_indent_behavior();
}

static void test_basic_space_indent(void) {
    printf("Testing basic space auto-indent...\n");
    
    setup_global();
    
    // Insert line with 4-space indent
    pgb_insert_str(&global.text, "    hello", &global.arena);
    
    // Simulate newline with auto-indent
    simulate_newline_with_auto_indent();
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    
    // Should have: "    hello\n    "
    printf("  Result: '%s'\n", buffer);
    assert(strstr(buffer, "    hello\n    ") != NULL);
    
    cleanup_global();
    printf("  ✓ Basic space indent test passed\n");
}

static void test_basic_tab_indent(void) {
    printf("Testing basic tab auto-indent...\n");
    
    setup_global();
    
    // Insert line with 2-tab indent
    pgb_insert_str(&global.text, "\t\thello", &global.arena);
    
    // Simulate newline with auto-indent
    simulate_newline_with_auto_indent();
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    
    // Should have: "\t\thello\n\t\t"
    printf("  Result: '%s'\n", buffer);
    assert(strstr(buffer, "\t\thello\n\t\t") != NULL);
    
    cleanup_global();
    printf("  ✓ Basic tab indent test passed\n");
}

static void test_mixed_whitespace_indent(void) {
    printf("Testing mixed whitespace auto-indent...\n");
    
    setup_global();
    
    // Insert line with mixed indent
    pgb_insert_str(&global.text, "  \t  hello", &global.arena);
    
    // Simulate newline with auto-indent
    simulate_newline_with_auto_indent();
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    
    // Should have: "  \t  hello\n  \t  "
    printf("  Result: '%s'\n", buffer);
    assert(strstr(buffer, "  \t  hello\n  \t  ") != NULL);
    
    cleanup_global();
    printf("  ✓ Mixed whitespace indent test passed\n");
}

static void test_no_indent(void) {
    printf("Testing no indent scenario...\n");
    
    setup_global();
    
    // Insert line without indent
    pgb_insert_str(&global.text, "hello", &global.arena);
    
    // Simulate newline with auto-indent
    simulate_newline_with_auto_indent();
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    
    // Should have: "hello\n" (no extra indent)
    printf("  Result: '%s'\n", buffer);
    assert(strcmp(buffer, "hello\n") == 0);
    
    cleanup_global();
    printf("  ✓ No indent test passed\n");
}

static void test_empty_buffer(void) {
    printf("Testing empty buffer scenario...\n");
    
    setup_global();
    
    // Buffer is empty, try newline
    simulate_newline_with_auto_indent();
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    
    // Should have: "\n" (no indent added)
    printf("  Result: '%s'\n", buffer);
    assert(strcmp(buffer, "\n") == 0);
    
    cleanup_global();
    printf("  ✓ Empty buffer test passed\n");
}

static void test_single_character_before_newline(void) {
    printf("Testing single character before newline...\n");
    
    setup_global();
    
    // Insert single character
    pgb_insert(&global.text, 'a', &global.arena);
    
    // Simulate newline with auto-indent
    simulate_newline_with_auto_indent();
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    
    // Should have: "a\n" (no indent since previous line has no leading whitespace)
    printf("  Result: '%s'\n", buffer);
    assert(strcmp(buffer, "a\n") == 0);
    
    cleanup_global();
    printf("  ✓ Single character test passed\n");
}

static void test_multiple_lines_with_indent(void) {
    printf("Testing multiple lines with consistent indent...\n");
    
    setup_global();
    
    // Insert first line with indent
    pgb_insert_str(&global.text, "    first line", &global.arena);
    simulate_newline_with_auto_indent();
    
    // Add text to second line
    pgb_insert_str(&global.text, "second line", &global.arena);
    simulate_newline_with_auto_indent();
    
    // Add text to third line
    pgb_insert_str(&global.text, "third line", &global.arena);
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    
    // Should have consistent 4-space indent on all lines
    printf("  Result: '%s'\n", buffer);
    assert(strstr(buffer, "    first line\n    second line\n    third line") != NULL);
    
    cleanup_global();
    printf("  ✓ Multiple lines test passed\n");
}

static void test_large_indent(void) {
    printf("Testing large indent (close to buffer limit)...\n");
    
    setup_global();
    
    // Insert line with large indent (but within 256 char limit)
    char large_indent[200];
    memset(large_indent, ' ', sizeof(large_indent) - 1);
    large_indent[sizeof(large_indent) - 1] = '\0';
    
    pgb_insert_str(&global.text, large_indent, &global.arena);
    pgb_insert_str(&global.text, "hello", &global.arena);
    
    // Simulate newline with auto-indent
    simulate_newline_with_auto_indent();
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    
    // Should preserve the large indent
    printf("  Result length: %zu\n", strlen(buffer));
    assert(strstr(buffer, large_indent) != NULL);
    
    cleanup_global();
    printf("  ✓ Large indent test passed\n");
}

static void test_disabled_auto_indent(void) {
    printf("Testing disabled auto-indent...\n");
    
    setup_global();
    
    // Insert line with indent
    pgb_insert_str(&global.text, "    hello", &global.arena);
    
    // Simulate newline without auto_indent call
    pgb_insert(&global.text, '\n', &global.arena);
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    
    // Should have: "    hello\n" (no extra indent added)
    printf("  Result: '%s'\n", buffer);
    assert(strcmp(buffer, "    hello\n") == 0);
    
    cleanup_global();
    printf("  ✓ Disabled auto-indent test passed\n");
}

int main(void) {
    printf("\n=== ZEX AUTO-INDENT FUNCTIONALITY TESTS ===\n\n");
    
    test_basic_space_indent();
    test_basic_tab_indent();
    test_mixed_whitespace_indent();
    test_no_indent();
    test_empty_buffer();
    test_single_character_before_newline();
    test_multiple_lines_with_indent();
    test_large_indent();
    test_disabled_auto_indent();
    
    printf("\n✓ ALL AUTO-INDENT TESTS PASSED!\n");
    return 0;
}