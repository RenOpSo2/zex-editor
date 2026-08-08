#include "src/editor.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define VERSION "0.2.0-pre"

void print_version() {
    printf("Noxe-Editor version %s\n", VERSION);
    printf("MIT License\n");
    printf("Copyright (c) 2026 Revan Aprian\n");	
}

int main(int argc, char* argv[]) {
    // Check for --version flag
    if (argc > 1 && strcmp(argv[1], "--version") == 0) {
        print_version();
        return 0;
    }
    
    static struct global global;
    const char* filepath = (argc > 1) ? argv[1] : NULL;
    editor_init(&global, filepath);

    editor_update(&global);

    editor_deinit(&global);
    return 0;
}



