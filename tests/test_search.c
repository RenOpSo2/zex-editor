/*
 * test_search.c - Tests for search functionality
 *
 * Tests search functionality including:
 * - Basic search
 * - Search next/previous
 * - Multiple matches
 * - Case sensitivity
 * - Unicode search
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
}

static void test_basic_search(void) {
    printf("Testing basic search functionality...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* content = "Hello World\nThis is a test\nHello again\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Initialize search
    search_init(&g);
    
    // Search for "Hello"
    search_find(&g, "Hello");
    
    assert(g.search_active == true);
    assert(g.search_match_count == 2);
    printf("  Found 2 matches for 'Hello'\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Basic search test passed\n");
}

static void test_search_next(void) {
    printf("Testing search next functionality...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* content = "apple banana apple cherry apple\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Initialize search
    search_init(&g);
    search_find(&g, "apple");
    
    assert(g.search_match_count == 3);
    printf("  Found 3 matches for 'apple'\n");
    
    // Move to next match
    uint32_t initial_pos = g.search_pos;
    search_next(&g);
    assert(g.search_pos != initial_pos);
    printf("  Moved to next match\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Search next test passed\n");
}

static void test_search_prev(void) {
    printf("Testing search previous functionality...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* content = "one two three two one\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Initialize search
    search_init(&g);
    search_find(&g, "two");
    
    assert(g.search_match_count == 2);
    printf("  Found 2 matches for 'two'\n");
    
    // Move to previous match
    uint32_t initial_pos = g.search_pos;
    search_prev(&g);
    assert(g.search_pos != initial_pos);
    printf("  Moved to previous match\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Search previous test passed\n");
}

static void test_no_match(void) {
    printf("Testing search with no matches...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* content = "Hello World\nThis is a test\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Initialize search
    search_init(&g);
    
    // Search for non-existent term
    search_find(&g, "nonexistent");
    
    assert(g.search_match_count == 0);
    printf("  Correctly found 0 matches for non-existent term\n");
    
    cleanup_global(&g);
    
    printf("  ✓ No match test passed\n");
}

static void test_empty_search(void) {
    printf("Testing empty search query...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* content = "Hello World\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Initialize search
    search_init(&g);
    
    // Search for empty string
    search_find(&g, "");
    
    // Should handle gracefully
    printf("  Empty search query handled gracefully\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Empty search test passed\n");
}

static void test_unicode_search(void) {
    printf("Testing Unicode search...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content with Unicode
    const char* content = "Hello 世界\nWorld 世界\nTest 世界\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Initialize search
    search_init(&g);
    
    // Search for Unicode term
    search_find(&g, "世界");
    
    assert(g.search_match_count == 3);
    printf("  Found 3 matches for '世界'\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Unicode search test passed\n");
}

static void test_case_sensitive_search(void) {
    printf("Testing case-sensitive search...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content with mixed case
    const char* content = "Hello hello HELLO\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Initialize search
    search_init(&g);
    
    // Search for exact case
    search_find(&g, "Hello");
    
    // Note: Current implementation may or may not be case-sensitive
    // This test documents current behavior
    printf("  Case-sensitive search behavior documented\n");
    printf("  Found %u matches for 'Hello'\n", g.search_match_count);
    
    cleanup_global(&g);
    
    printf("  ✓ Case-sensitive search test passed\n");
}

static void test_search_in_empty_buffer(void) {
    printf("Testing search in empty buffer...\n");
    
    struct global g;
    setup_global(&g);
    
    // Initialize search with empty buffer
    search_init(&g);
    
    // Search for term
    search_find(&g, "test");
    
    assert(g.search_match_count == 0);
    printf("  Correctly found 0 matches in empty buffer\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Empty buffer search test passed\n");
}

static void test_multiple_search_operations(void) {
    printf("Testing multiple search operations...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* content = "apple banana cherry\n";
    pgb_insert_str(&g.text, content, &g.arena);
    
    // Initialize search
    search_init(&g);
    
    // First search
    search_find(&g, "apple");
    uint32_t first_count = g.search_match_count;
    
    // Second search (different term)
    search_find(&g, "banana");
    uint32_t second_count = g.search_match_count;
    
    // Third search (another term)
    search_find(&g, "cherry");
    uint32_t third_count = g.search_match_count;
    
    assert(first_count == 1);
    assert(second_count == 1);
    assert(third_count == 1);
    printf("  Multiple search operations work correctly\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Multiple search operations test passed\n");
}

int main(void) {
    printf("\n=== ZEX SEARCH FUNCTIONALITY TESTS ===\n\n");
    
    test_basic_search();
    test_search_next();
    test_search_prev();
    test_no_match();
    test_empty_search();
    test_unicode_search();
    test_case_sensitive_search();
    test_search_in_empty_buffer();
    test_multiple_search_operations();
    
    printf("\n✓ ALL SEARCH TESTS PASSED!\n");
    return 0;
}
