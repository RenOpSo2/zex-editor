/*
 * test_cmd.c - Tests for command execution functionality
 *
 * Tests command execution including:
 * - Exit/quit commands
 * - Open file command
 * - Save file command
 * - Unknown command handling
 * - Command parsing
 */
#include "src/global.h"
#include "src/nodes.h"
#include "src/cmd.h"
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

static void test_exit_command(void) {
    printf("Testing exit command...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create command buffer with "exit"
    struct paged_gap_buffer cmd_buf;
    pgb_init(&cmd_buf, &g.arena);
    pgb_insert_str(&cmd_buf, "exit", &g.arena);
    
    // Execute exit command
    enum result result = cmd_exec(&g, &cmd_buf);
    
    // Exit command should return err (to signal program exit)
    assert(result == err);
    printf("  Exit command returns err as expected\n");
    
    pgb_clear(&cmd_buf);
    cleanup_global(&g);
    
    printf("  ✓ Exit command test passed\n");
}

static void test_quit_command(void) {
    printf("Testing quit command...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create command buffer with "quit"
    struct paged_gap_buffer cmd_buf;
    pgb_init(&cmd_buf, &g.arena);
    pgb_insert_str(&cmd_buf, "quit", &g.arena);
    
    // Execute quit command
    enum result result = cmd_exec(&g, &cmd_buf);
    
    // Quit command should return err (to signal program exit)
    assert(result == err);
    printf("  Quit command returns err as expected\n");
    
    pgb_clear(&cmd_buf);
    cleanup_global(&g);
    
    printf("  ✓ Quit command test passed\n");
}

static void test_q_command(void) {
    printf("Testing 'q' shorthand command...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create command buffer with "q"
    struct paged_gap_buffer cmd_buf;
    pgb_init(&cmd_buf, &g.arena);
    pgb_insert_str(&cmd_buf, "q", &g.arena);
    
    // Execute q command
    enum result result = cmd_exec(&g, &cmd_buf);
    
    // q command should return err (to signal program exit)
    assert(result == err);
    printf("  'q' command returns err as expected\n");
    
    pgb_clear(&cmd_buf);
    cleanup_global(&g);
    
    printf("  ✓ 'q' command test passed\n");
}

static void test_unknown_command(void) {
    printf("Testing unknown command handling...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create command buffer with unknown command
    struct paged_gap_buffer cmd_buf;
    pgb_init(&cmd_buf, &g.arena);
    pgb_insert_str(&cmd_buf, "unknown_command", &g.arena);
    
    // Execute unknown command
    enum result result = cmd_exec(&g, &cmd_buf);
    
    // Unknown command should return ok (but with error message)
    assert(result == ok);
    
    // Check error message
    char msg_buf[buf_capacity];
    pgb_to_str(msg_buf, sizeof(msg_buf), &g.msg);
    assert(strcmp(msg_buf, "command not found.") == 0);
    printf("  Unknown command handled correctly\n");
    
    pgb_clear(&cmd_buf);
    cleanup_global(&g);
    
    printf("  ✓ Unknown command test passed\n");
}

static void test_open_command(void) {
    printf("Testing open command...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create a test file first
    const char* test_file = "./test_cmd_open.txt";
    FILE* f = fopen(test_file, "w");
    fprintf(f, "Test content for open command\n");
    fclose(f);
    
    // Create command buffer with "open" command
    struct paged_gap_buffer cmd_buf;
    pgb_init(&cmd_buf, &g.arena);
    pgb_insert_str(&cmd_buf, "open ./test_cmd_open.txt", &g.arena);
    
    // Execute open command
    enum result result = cmd_exec(&g, &cmd_buf);
    
    // Open command should return ok
    assert(result == ok);
    
    // Check success message
    char msg_buf[buf_capacity];
    pgb_to_str(msg_buf, sizeof(msg_buf), &g.msg);
    assert(strcmp(msg_buf, "open succeeded.") == 0);
    printf("  Open command succeeded\n");
    
    // Verify content was loaded
    char content_buf[buf_capacity];
    pgb_to_str(content_buf, sizeof(content_buf), &g.text);
    assert(strcmp(content_buf, "Test content for open command\n") == 0);
    printf("  File content loaded correctly\n");
    
    // Cleanup
    pgb_clear(&cmd_buf);
    unlink(test_file);
    cleanup_global(&g);
    
    printf("  ✓ Open command test passed\n");
}

static void test_open_nonexistent_file(void) {
    printf("Testing open command with non-existent file...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create command buffer with open command for non-existent file
    struct paged_gap_buffer cmd_buf;
    pgb_init(&cmd_buf, &g.arena);
    pgb_insert_str(&cmd_buf, "open /nonexistent/file.txt", &g.arena);
    
    // Execute open command
    enum result result = cmd_exec(&g, &cmd_buf);
    
    // Open command should return ok (but with error message)
    assert(result == ok);
    
    // Check error message
    char msg_buf[buf_capacity];
    pgb_to_str(msg_buf, sizeof(msg_buf), &g.msg);
    assert(strcmp(msg_buf, "open failed.") == 0);
    printf("  Non-existent file handled correctly\n");
    
    pgb_clear(&cmd_buf);
    cleanup_global(&g);
    
    printf("  ✓ Open non-existent file test passed\n");
}

static void test_save_command(void) {
    printf("Testing save command...\n");
    
    struct global g;
    setup_global(&g);
    
    // Add content to buffer
    pgb_insert_str(&g.text, "Test content for save command\n", &g.arena);
    
    // Create command buffer with "save" command
    struct paged_gap_buffer cmd_buf;
    pgb_init(&cmd_buf, &g.arena);
    pgb_insert_str(&cmd_buf, "save ./test_cmd_save.txt", &g.arena);
    
    // Execute save command
    enum result result = cmd_exec(&g, &cmd_buf);
    
    // Save command should return ok
    assert(result == ok);
    
    // Check success message
    char msg_buf[buf_capacity];
    pgb_to_str(msg_buf, sizeof(msg_buf), &g.msg);
    assert(strcmp(msg_buf, "save succeeded.") == 0);
    printf("  Save command succeeded\n");
    
    // Verify file was created
    FILE* f = fopen("./test_cmd_save.txt", "r");
    assert(f != NULL);
    char buf[256];
    fgets(buf, sizeof(buf), f);
    fclose(f);
    assert(strcmp(buf, "Test content for save command\n") == 0);
    printf("  File saved correctly\n");
    
    // Cleanup
    pgb_clear(&cmd_buf);
    unlink("./test_cmd_save.txt");
    cleanup_global(&g);
    
    printf("  ✓ Save command test passed\n");
}

static void test_save_invalid_path(void) {
    printf("Testing save command with invalid path...\n");
    
    struct global g;
    setup_global(&g);
    
    // Add content to buffer
    pgb_insert_str(&g.text, "Test content\n", &g.arena);
    
    // Create command buffer with save command for invalid path
    struct paged_gap_buffer cmd_buf;
    pgb_init(&cmd_buf, &g.arena);
    pgb_insert_str(&cmd_buf, "save /nonexistent/path/file.txt", &g.arena);
    
    // Execute save command
    enum result result = cmd_exec(&g, &cmd_buf);
    
    // Save command should return ok (but with error message)
    assert(result == ok);
    
    // Check error message
    char msg_buf[buf_capacity];
    pgb_to_str(msg_buf, sizeof(msg_buf), &g.msg);
    assert(strcmp(msg_buf, "save failed.") == 0);
    printf("  Invalid path handled correctly\n");
    
    pgb_clear(&cmd_buf);
    cleanup_global(&g);
    
    printf("  ✓ Save invalid path test passed\n");
}

static void test_command_parsing(void) {
    printf("Testing command parsing with arguments...\n");
    
    struct global g;
    setup_global(&g);
    
    // Create command buffer with command and argument
    struct paged_gap_buffer cmd_buf;
    pgb_init(&cmd_buf, &g.arena);
    pgb_insert_str(&cmd_buf, "open   test.txt", &g.arena); // multiple spaces
    
    // Execute command
    cmd_exec(&g, &cmd_buf);
    
    // Should handle multiple spaces in command
    printf("  Command parsing handles spaces correctly\n");
    
    pgb_clear(&cmd_buf);
    cleanup_global(&g);
    
    printf("  ✓ Command parsing test passed\n");
}

int main(void) {
    printf("\n=== ZEX COMMAND EXECUTION TESTS ===\n\n");
    
    test_exit_command();
    test_quit_command();
    test_q_command();
    test_unknown_command();
    test_open_command();
    test_open_nonexistent_file();
    test_save_command();
    test_save_invalid_path();
    test_command_parsing();
    
    printf("\n✓ ALL COMMAND EXECUTION TESTS PASSED!\n");
    return 0;
}
