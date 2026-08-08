#include "src/config.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

int main() {
    printf("Running config system tests...\n");

    // Test 1: init defaults
    config_init();
    assert(config_get_number("tabsize", 4) == 4.0);
    assert(config_get_bool("mouse", 0) == 1);
    assert(config_get_bool("show_line_numbers", 0) == 1);
    assert(config_get_bool("auto_indent", 0) == 1);
    printf("Test 1 (Defaults): Passed.\n");

    // Test 2: Validation
    SchemaError err;
    assert(config_validate("tabsize", "abc", &err) == 0);
    assert(config_validate("tabsize", "4", NULL) == 1);
    assert(config_validate("mouse", "invalid", &err) == 0);
    assert(config_validate("mouse", "true", NULL) == 1);
    assert(config_validate("show_line_numbers", "invalid", &err) == 0);
    assert(config_validate("show_line_numbers", "false", NULL) == 1);
    assert(config_validate("auto_indent", "invalid", &err) == 0);
    assert(config_validate("auto_indent", "true", NULL) == 1);
    printf("Test 2 (Validation): Passed.\n");

    // Test 3: Load from CLI overrides
    char* test_argv[] = {
        "./zex",
        "--tabsize", "8",
        "--mouse", "false",
        "--show_line_numbers", "false",
        "--auto_indent", "false",
        "--custom_theme", "dracula"
    };
    int test_argc = sizeof(test_argv) / sizeof(test_argv[0]);
    config_load(test_argc, test_argv);
    
    assert(config_get_number("tabsize", 4) == 8.0);
    assert(config_get_bool("mouse", 1) == 0);
    assert(config_get_bool("show_line_numbers", 1) == 0);
    assert(config_get_bool("auto_indent", 1) == 0);
    assert(strcmp(config_get_string("custom_theme", ""), "dracula") == 0);
    printf("Test 3 (CLI Override): Passed.\n");

    // Test 4: Set and file write
    // Force output to ./zex.json for test isolation
    config_set_filepath("./zex.json");
    config_set_number("tabsize", 2);
    config_set_bool("mouse", 1);
    config_set_bool("show_line_numbers", 0);
    config_set_bool("auto_indent", 0);

    // Read directly from file to verify JSON format
    FILE* f = fopen("./zex.json", "r");
    assert(f != NULL);
    char buf[1024];
    size_t r = fread(buf, 1, sizeof(buf) - 1, f);
    buf[r] = '\0';
    fclose(f);

    assert(strstr(buf, "\"tabsize\": 2") != NULL);
    assert(strstr(buf, "\"mouse\": true") != NULL);
    assert(strstr(buf, "\"show_line_numbers\": false") != NULL);
    assert(strstr(buf, "\"auto_indent\": false") != NULL);
    printf("Test 4 (File Persistence/JSON format): Passed.\n");

    // Clean up
    unlink("./zex.json");

    printf("All config system tests passed successfully!\n");
    return 0;
}
