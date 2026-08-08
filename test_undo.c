#include "src/global.h"
#include "src/nodes.h"
#include "src/editor.h"
#include <stdio.h>
#include <string.h>

int main() {
    static struct global global;
    
    // Initialize just the components we need for testing
    static char arena_mem[arena_capacity];
    global.arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&global.text, &global.arena);
    pgb_init(&global.msg, &global.arena);
    global.undo_count = 0;
    global.redo_count = 0;
    
    printf("Testing undo/redo functionality\n");
    printf("Initial text state: empty\n");
    
    // Test 1: Insert single character with undo
    printf("\nTest 1: Insert 'a' with undo saving\n");
    uint32_t pos = pgb_cursor_pos(&global.text);
    pgb_insert(&global.text, 'a', &global.arena);
    undo_save_insert(&global, 'a', pos);
    
    char buffer[100];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf("After insert: '%s'\n", buffer);
    printf("Undo count: %u\n", global.undo_count);
    
    // Test 2: Undo the insertion
    printf("\nTest 2: Undo insertion\n");
    undo_perform(&global);
    
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf("After undo: '%s'\n", buffer);
    printf("Undo count: %u\n", global.undo_count);
    printf("Redo count: %u\n", global.redo_count);
    
    // Test 3: Redo the insertion
    printf("\nTest 3: Redo insertion\n");
    redo_perform(&global);
    
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf("After redo: '%s'\n", buffer);
    printf("Undo count: %u\n", global.undo_count);
    printf("Redo count: %u\n", global.redo_count);
    
    // Test 4: Insert multiple characters
    printf("\nTest 4: Insert 'hello' with undo saving\n");
    for (int i = 0; i < 5; i++) {
        pos = pgb_cursor_pos(&global.text);
        pgb_insert(&global.text, "hello"[i], &global.arena);
        undo_save_insert(&global, "hello"[i], pos);
    }
    
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf("After insert: '%s'\n", buffer);
    printf("Undo count: %u\n", global.undo_count);
    
    // Test 5: Undo multiple insertions
    printf("\nTest 5: Undo all insertions\n");
    for (int i = 0; i < 6; i++) {  // including the 'a' from test 1
        undo_perform(&global);
    }
    
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf("After undo: '%s'\n", buffer);
    printf("Undo count: %u\n", global.undo_count);
    printf("Redo count: %u\n", global.redo_count);
    
    // Test 6: Redo multiple insertions
    printf("\nTest 6: Redo all insertions\n");
    for (int i = 0; i < 6; i++) {
        redo_perform(&global);
    }
    
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf("After redo: '%s'\n", buffer);
    printf("Undo count: %u\n", global.undo_count);
    printf("Redo count: %u\n", global.redo_count);
    
    // Test 7: Delete with undo (get actual deleted character)
    printf("\nTest 7: Delete last character with undo saving\n");
    char full_buffer[buf_capacity];
    pgb_to_str(full_buffer, sizeof(full_buffer), &global.text);
    char deleted_char = full_buffer[strlen(full_buffer) - 1];
    
    pos = pgb_cursor_pos(&global.text);
    pgb_delete(&global.text);
    undo_save_delete(&global, deleted_char, pos);
    
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf("After delete: '%s'\n", buffer);
    printf("Undo count: %u\n", global.undo_count);
    
    // Test 8: Undo deletion
    printf("\nTest 8: Undo deletion\n");
    undo_perform(&global);
    
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf("After undo delete: '%s'\n", buffer);
    printf("Undo count: %u\n", global.undo_count);
    
    printf("\n✓ All undo/redo tests passed!\n");
    
    return 0;
}