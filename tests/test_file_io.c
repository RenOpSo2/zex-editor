/*
 * test_file_io.c - Tests for file I/O operations
 *
 * Tests file reading and writing functionality including:
 * - Basic file read/write
 * - File validation and security
 * - Large file handling
 * - Unicode content handling
 */
#include "src/global.h"
#include "src/nodes.h"
#include "src/file.h"
#include "src/editor.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

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

static void cleanup_global(struct global* g) {
    pgb_clear(&g->text);
    pgb_clear(&g->msg);
}

static void test_basic_write_read(void) {
    printf("Testing basic file write and read...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content
    const char* test_content = "Hello, World!\nThis is a test file.\nLine 3.";
    pgb_insert_str(&g.text, test_content, &g.arena);
    
    // Write to file
    const char* test_file = "./test_file_io_tmp.txt";
    enum result write_result = file_write(test_file, &g.text);
    assert(write_result == ok);
    printf("  File write succeeded\n");
    
    // Clear buffer
    pgb_clear(&g.text);
    
    // Read back
    enum result read_result = file_read(&g.text, test_file, &g.arena);
    assert(read_result == ok);
    printf("  File read succeeded\n");
    
    // Verify content
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &g.text);
    assert(strcmp(buffer, test_content) == 0);
    printf("  Content matches original\n");
    
    // Cleanup
    unlink(test_file);
    cleanup_global(&g);
    
    printf("  ✓ Basic write/read test passed\n");
}

static void test_unicode_content(void) {
    printf("Testing Unicode content in files...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create test content with Unicode
    const char* unicode_content = "Hello 世界\nEmoji: 😀\nLatin: café\n";
    pgb_insert_str(&g.text, unicode_content, &g.arena);
    
    // Write to file
    const char* test_file = "./test_unicode_tmp.txt";
    enum result write_result = file_write(test_file, &g.text);
    assert(write_result == ok);
    printf("  Unicode file write succeeded\n");
    
    // Clear and read back
    pgb_clear(&g.text);
    enum result read_result = file_read(&g.text, test_file, &g.arena);
    assert(read_result == ok);
    printf("  Unicode file read succeeded\n");
    
    // Verify content
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &g.text);
    assert(strcmp(buffer, unicode_content) == 0);
    printf("  Unicode content matches original\n");
    
    // Cleanup
    unlink(test_file);
    cleanup_global(&g);
    
    printf("  ✓ Unicode content test passed\n");
}

static void test_empty_file(void) {
    printf("Testing empty file handling...\n");
    
    struct global g;
    setup_global(&g);
    
    // Write empty file
    const char* test_file = "./test_empty_tmp.txt";
    enum result write_result = file_write(test_file, &g.text);
    assert(write_result == ok);
    printf("  Empty file write succeeded\n");
    
    // Read back
    pgb_clear(&g.text);
    enum result read_result = file_read(&g.text, test_file, &g.arena);
    assert(read_result == ok);
    printf("  Empty file read succeeded\n");
    
    // Verify it's empty
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &g.text);
    assert(strlen(buffer) == 0);
    printf("  File is empty as expected\n");
    
    // Cleanup
    unlink(test_file);
    cleanup_global(&g);
    
    printf("  ✓ Empty file test passed\n");
}

static void test_large_file(void) {
    printf("Testing large file handling...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create a larger file (multiple pages)
    const char* line = "This is a test line with some content.\n";
    for (int i = 0; i < 100; i++) {
        pgb_insert_str(&g.text, line, &g.arena);
    }
    
    const char* test_file = "./test_large_tmp.txt";
    enum result write_result = file_write(test_file, &g.text);
    assert(write_result == ok);
    printf("  Large file write succeeded\n");
    
    // Clear and read back
    pgb_clear(&g.text);
    enum result read_result = file_read(&g.text, test_file, &g.arena);
    assert(read_result == ok);
    printf("  Large file read succeeded\n");
    
    // Verify line count
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &g.text);
    int line_count = 0;
    for (int i = 0; buffer[i]; i++) {
        if (buffer[i] == '\n') line_count++;
    }
    assert(line_count == 100);
    printf("  All 100 lines preserved\n");
    
    // Cleanup
    unlink(test_file);
    cleanup_global(&g);
    
    printf("  ✓ Large file test passed\n");
}

static void test_invalid_path(void) {
    printf("Testing invalid path handling...\n");
    
    struct global g;
    setup_global(&g);
    
    // Try to read non-existent file
    enum result read_result = file_read(&g.text, "/nonexistent/path/file.txt", &g.arena);
    assert(read_result == err);
    printf("  Non-existent file correctly rejected\n");
    
    // Try to write to invalid path
    pgb_insert_str(&g.text, "test", &g.arena);
    enum result write_result = file_write("/nonexistent/path/file.txt", &g.text);
    assert(write_result == err);
    printf("  Invalid write path correctly rejected\n");
    
    cleanup_global(&g);
    
    printf("  ✓ Invalid path test passed\n");
}

static void test_overwrite_file(void) {
    printf("Testing file overwrite...\n");
    
    struct global g;
    setup_global(&g);
    
    const char* test_file = "./test_overwrite_tmp.txt";
    
    // Write initial content
    pgb_insert_str(&g.text, "Initial content\n", &g.arena);
    file_write(test_file, &g.text);
    printf("  Initial file written\n");
    
    // Overwrite with new content
    pgb_clear(&g.text);
    pgb_insert_str(&g.text, "New content\n", &g.arena);
    enum result write_result = file_write(test_file, &g.text);
    assert(write_result == ok);
    printf("  File overwritten\n");
    
    // Read back and verify
    pgb_clear(&g.text);
    file_read(&g.text, test_file, &g.arena);
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &g.text);
    assert(strcmp(buffer, "New content\n") == 0);
    printf("  New content verified\n");
    
    // Cleanup
    unlink(test_file);
    cleanup_global(&g);
    
    printf("  ✓ File overwrite test passed\n");
}

int main(void) {
    printf("\n=== ZEX FILE I/O TESTS ===\n\n");
    
    test_basic_write_read();
    test_unicode_content();
    test_empty_file();
    test_large_file();
    test_invalid_path();
    test_overwrite_file();
    
    printf("\n✓ ALL FILE I/O TESTS PASSED!\n");
    return 0;
}
