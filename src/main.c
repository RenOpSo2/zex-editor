#include "editor.h"
#include "config.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>


#define VERSION "1.0.0"

void print_version() {
    printf("Zex version %s\n", VERSION);
    printf("GPL-3 License\n");
    printf("Copyright (c) 2026 Revan Aprian\n");
}

int main(int argc, char* argv[]) {
    // Check for --version flag
    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        print_version();
        return 0;
    }
    
    // Load config from ~/.zexrc, ./zex.json, and CLI args
    config_load(argc, argv);
    
    static struct global global;
    const char* filepath = NULL;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--", 2) == 0) {
            i++; // skip value
        } else {
            filepath = argv[i];
            break;
        }
    }
    
    editor_init(&global, filepath);
    while (1) {
        config_watch(&global);
        if (editor_update(&global) != ok) {
            break;
        }
    }
    editor_deinit(&global);
    return 0;
}
