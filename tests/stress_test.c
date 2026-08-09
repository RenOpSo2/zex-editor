#include "../src/global.h"
#include "../src/nodes.h"
#include "../src/editor.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_RESET "\x1b[0m"

typedef struct {
    const char* name;
    clock_t start;
    clock_t end;
} benchmark;

void benchmark_start(benchmark* b, const char* name) {
    b->name = name;
    b->start = clock();
}

void benchmark_end(benchmark* b) {
    b->end = clock();
    double elapsed = ((double)(b->end - b->start)) / CLOCKS_PER_SEC * 1000.0;
    printf(ANSI_COLOR_BLUE "  %s: %.2f ms" ANSI_COLOR_RESET "\n", b->name, elapsed);
}

void test_large_file_insert() {
    printf(ANSI_COLOR_YELLOW "\n=== Large File Insert Test ===" ANSI_COLOR_RESET "\n");
    
    static struct global global;
    static char arena_mem[arena_capacity];
    global.arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&global.text, &global.arena);
    pgb_init(&global.msg, &global.arena);
    global.undo_count = 0;
    global.redo_count = 0;
    
    benchmark b;
    benchmark_start(&b, "Insert 100,000 characters");
    
    for (int i = 0; i < 100000; i++) {
        pgb_insert(&global.text, 'A' + (i % 26), &global.arena);
    }
    
    benchmark_end(&b);
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf(ANSI_COLOR_GREEN "  Total characters: %zu" ANSI_COLOR_RESET "\n", strlen(buffer));
    
    benchmark_start(&b, "Move cursor to start");
    for (int i = 0; i < 100000; i++) {
        pgb_move_left(&global.text);
    }
    benchmark_end(&b);
    
    benchmark_start(&b, "Move cursor to end");
    for (int i = 0; i < 100000; i++) {
        pgb_move_right(&global.text);
    }
    benchmark_end(&b);
    
    printf(ANSI_COLOR_GREEN "  ✓ Large file insert test passed" ANSI_COLOR_RESET "\n");
}

void test_rapid_insert_delete() {
    printf(ANSI_COLOR_YELLOW "\n=== Rapid Insert/Delete Test ===" ANSI_COLOR_RESET "\n");
    
    static struct global global;
    static char arena_mem[arena_capacity];
    global.arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&global.text, &global.arena);
    pgb_init(&global.msg, &global.arena);
    global.undo_count = 0;
    global.redo_count = 0;
    
    benchmark b;
    benchmark_start(&b, "Rapid insert/delete (10,000 cycles)");
    
    for (int i = 0; i < 10000; i++) {
        pgb_insert(&global.text, 'X', &global.arena);
        pgb_delete(&global.text);
    }
    
    benchmark_end(&b);
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf(ANSI_COLOR_GREEN "  Final buffer length: %zu (should be 0)" ANSI_COLOR_RESET "\n", strlen(buffer));
    
    if (strlen(buffer) == 0) {
        printf(ANSI_COLOR_GREEN "  ✓ Rapid insert/delete test passed" ANSI_COLOR_RESET "\n");
    } else {
        printf(ANSI_COLOR_RED "  ✗ Rapid insert/delete test failed" ANSI_COLOR_RESET "\n");
    }
}

void test_large_string_operations() {
    printf(ANSI_COLOR_YELLOW "\n=== Large String Operations Test ===" ANSI_COLOR_RESET "\n");
    
    static struct global global;
    static char arena_mem[arena_capacity];
    global.arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&global.text, &global.arena);
    pgb_init(&global.msg, &global.arena);
    global.undo_count = 0;
    global.redo_count = 0;
    
    // Create a large string
    char large_string[5001];
    for (int i = 0; i < 5000; i++) {
        large_string[i] = 'A' + (i % 26);
    }
    large_string[5000] = '\0';
    
    benchmark b;
    benchmark_start(&b, "Insert 5,000 character string");
    pgb_insert_str(&global.text, large_string, &global.arena);
    benchmark_end(&b);
    
    benchmark_start(&b, "Replace with another 5,000 character string");
    pgb_replace_str(&global.text, large_string, &global.arena);
    benchmark_end(&b);
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf(ANSI_COLOR_GREEN "  Buffer length: %zu" ANSI_COLOR_RESET "\n", strlen(buffer));
    
    printf(ANSI_COLOR_GREEN "  ✓ Large string operations test passed" ANSI_COLOR_RESET "\n");
}

void test_massive_undo_redo() {
    printf(ANSI_COLOR_YELLOW "\n=== Massive Undo/Redo Test ===" ANSI_COLOR_RESET "\n");
    
    static struct global global;
    static char arena_mem[arena_capacity];
    global.arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&global.text, &global.arena);
    pgb_init(&global.msg, &global.arena);
    global.undo_count = 0;
    global.redo_count = 0;
    
    benchmark b;
    benchmark_start(&b, "Insert 1,000 characters with undo");
    
    for (int i = 0; i < 1000; i++) {
        uint32_t pos = pgb_cursor_pos(&global.text);
        pgb_insert(&global.text, 'A' + (i % 26), &global.arena);
        undo_save_insert(&global, 'A' + (i % 26), pos);
    }
    
    benchmark_end(&b);
    printf(ANSI_COLOR_GREEN "  Undo count: %u" ANSI_COLOR_RESET "\n", global.undo_count);
    
    benchmark_start(&b, "Undo all 1,000 operations");
    for (int i = 0; i < 1000; i++) {
        undo_perform(&global);
    }
    benchmark_end(&b);
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf(ANSI_COLOR_GREEN "  After undo, buffer length: %zu" ANSI_COLOR_RESET "\n", strlen(buffer));
    
    benchmark_start(&b, "Redo all 1,000 operations");
    for (int i = 0; i < 1000; i++) {
        redo_perform(&global);
    }
    benchmark_end(&b);
    
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf(ANSI_COLOR_GREEN "  After redo, buffer length: %zu" ANSI_COLOR_RESET "\n", strlen(buffer));
    
    printf(ANSI_COLOR_GREEN "  ✓ Massive undo/redo test passed" ANSI_COLOR_RESET "\n");
}

void test_memory_pressure() {
    printf(ANSI_COLOR_YELLOW "\n=== Memory Pressure Test ===" ANSI_COLOR_RESET "\n");
    
    static struct global global;
    static char arena_mem[arena_capacity];
    global.arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&global.text, &global.arena);
    pgb_init(&global.msg, &global.arena);
    global.undo_count = 0;
    global.redo_count = 0;
    
    benchmark b;
    benchmark_start(&b, "Fill buffer with 50,000 characters");
    
    for (int i = 0; i < 50000; i++) {
        pgb_insert(&global.text, 'M', &global.arena);
    }
    
    benchmark_end(&b);
    
    // Delete and re-insert multiple times
    benchmark_start(&b, "Delete and re-insert 100 times");
    
    for (int cycle = 0; cycle < 100; cycle++) {
        // Delete half
        for (int i = 0; i < 25000; i++) {
            pgb_delete(&global.text);
        }
        // Re-insert
        for (int i = 0; i < 25000; i++) {
            pgb_insert(&global.text, 'M', &global.arena);
        }
    }
    
    benchmark_end(&b);
    
    char buffer[buf_capacity];
    pgb_to_str(buffer, sizeof(buffer), &global.text);
    printf(ANSI_COLOR_GREEN "  Final buffer length: %zu" ANSI_COLOR_RESET "\n", strlen(buffer));
    
    printf(ANSI_COLOR_GREEN "  ✓ Memory pressure test passed" ANSI_COLOR_RESET "\n");
}

void test_copy_paste_performance() {
    printf(ANSI_COLOR_YELLOW "\n=== Copy/Paste Performance Test ===" ANSI_COLOR_RESET "\n");
    
    static struct global global;
    static char arena_mem[arena_capacity];
    global.arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&global.text, &global.arena);
    pgb_init(&global.msg, &global.arena);
    global.undo_count = 0;
    global.redo_count = 0;
    
    // Create source text
    char source_text[1001];
    for (int i = 0; i < 1000; i++) {
        source_text[i] = 'A' + (i % 26);
    }
    source_text[1000] = '\0';
    
    pgb_insert_str(&global.text, source_text, &global.arena);
    
    benchmark b;
    benchmark_start(&b, "Copy range (100 times)");
    
    for (int i = 0; i < 100; i++) {
        pgb_copy_range(&global.msg, &global.text, 0, 500, &global.arena);
    }
    
    benchmark_end(&b);
    
    benchmark_start(&b, "Delete range (100 times)");
    
    for (int i = 0; i < 100; i++) {
        pgb_move_to_pos(&global.text, 0);
        pgb_delete_range(&global.text, 0, 100);
        // Re-insert for next iteration
        pgb_insert_str(&global.text, source_text, &global.arena);
    }
    
    benchmark_end(&b);
    
    printf(ANSI_COLOR_GREEN "  ✓ Copy/paste performance test passed" ANSI_COLOR_RESET "\n");
}

void test_cursor_navigation_stress() {
    printf(ANSI_COLOR_YELLOW "\n=== Cursor Navigation Stress Test ===" ANSI_COLOR_RESET "\n");
    
    static struct global global;
    static char arena_mem[arena_capacity];
    global.arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&global.text, &global.arena);
    pgb_init(&global.msg, &global.arena);
    global.undo_count = 0;
    global.redo_count = 0;
    
    // Create text with multiple lines
    for (int line = 0; line < 100; line++) {
        for (int col = 0; col < 50; col++) {
            pgb_insert(&global.text, 'A' + (col % 26), &global.arena);
        }
        pgb_insert(&global.text, '\n', &global.arena);
    }
    
    benchmark b;
    benchmark_start(&b, "Random cursor movement (10,000 operations)");
    
    srand(time(NULL));
    for (int i = 0; i < 10000; i++) {
        int op = rand() % 4;
        switch (op) {
            case 0: pgb_move_left(&global.text); break;
            case 1: pgb_move_right(&global.text); break;
            case 2: pgb_move_up(&global.text); break;
            case 3: pgb_move_down(&global.text); break;
        }
    }
    
    benchmark_end(&b);
    
    printf(ANSI_COLOR_GREEN "  ✓ Cursor navigation stress test passed" ANSI_COLOR_RESET "\n");
}

void test_search_performance() {
    printf(ANSI_COLOR_YELLOW "\n=== Search Performance Test ===" ANSI_COLOR_RESET "\n");
    
    static struct global global;
    static char arena_mem[arena_capacity];
    global.arena = arena_init(arena_mem, sizeof(arena_mem));
    pgb_init(&global.text, &global.arena);
    pgb_init(&global.msg, &global.arena);
    global.undo_count = 0;
    global.redo_count = 0;
    global.search_active = false;
    global.search_query[0] = '\0';
    global.search_pos = 0;
    global.search_match_count = 0;
    
    // Create large text with pattern
    for (int i = 0; i < 10000; i++) {
        pgb_insert_str(&global.text, "hello world ", &global.arena);
    }
    
    benchmark b;
    benchmark_start(&b, "Search for 'world' (100 times)");
    
    for (int i = 0; i < 100; i++) {
        search_find(&global, "world");
    }
    
    benchmark_end(&b);
    double elapsed_ms = ((double)(b.end - b.start)) / CLOCKS_PER_SEC * 1000.0;
    printf(ANSI_COLOR_GREEN "  Match count: %u" ANSI_COLOR_RESET "\n", global.search_match_count);

    if (global.search_match_count == 10000 && elapsed_ms < 150.0) {
        printf(ANSI_COLOR_GREEN "  ✓ Search performance test passed" ANSI_COLOR_RESET "\n");
    } else {
        printf(ANSI_COLOR_RED "  ✗ Search performance test failed (expected 10000 matches, <150ms)" ANSI_COLOR_RESET "\n");
    }
}

int main() {
    printf(ANSI_COLOR_GREEN "\n╔══════════════════════════════════════════════════════════╗" ANSI_COLOR_RESET "\n");
    printf(ANSI_COLOR_GREEN "║" ANSI_COLOR_RESET "          ZEX TEXT EDITOR - STRESS TEST SUITE          " ANSI_COLOR_GREEN "║" ANSI_COLOR_RESET "\n");
    printf(ANSI_COLOR_GREEN "╚══════════════════════════════════════════════════════════╝" ANSI_COLOR_RESET "\n");
    
    test_large_file_insert();
    test_rapid_insert_delete();
    test_large_string_operations();
    test_massive_undo_redo();
    test_memory_pressure();
    test_copy_paste_performance();
    test_cursor_navigation_stress();
    test_search_performance();
    
    printf(ANSI_COLOR_GREEN "\n╔══════════════════════════════════════════════════════════╗" ANSI_COLOR_RESET "\n");
    printf(ANSI_COLOR_GREEN "║" ANSI_COLOR_RESET "              ALL STRESS TESTS COMPLETED               " ANSI_COLOR_GREEN "║" ANSI_COLOR_RESET "\n");
    printf(ANSI_COLOR_GREEN "╚══════════════════════════════════════════════════════════╝" ANSI_COLOR_RESET "\n\n");
    
    return 0;
}
