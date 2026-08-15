#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "zui.h"

int main() {
    zui_init();
    zui_clear();
    
    ZuiSize size = zui_get_size();
    
    // Title
    ZuiLabel title = zui_label_new("=== ZUI Demo ===", 2, 2);
    zui_label_set_color(&title, ZUI_CYAN);
    zui_label_set_bold(&title, true);
    zui_label_draw(&title);
    
    // Box
    zui_draw_box(2, 4, 50, 12, " Menu ");
    
    // Menu items
    char *items[] = {"Option 1", "Option 2", "Option 3", "Exit"};
    ZuiMenu menu = zui_menu_new(items, 4, 6, 6);
    menu.color = ZUI_WHITE;
    menu.selected_color = ZUI_BLACK;
    menu.selected_bg = ZUI_BG_CYAN;
    zui_menu_draw(&menu);
    
    // Progress bar
    ZuiProgress progress = zui_progress_new(2, 18, 30, 100);
    zui_progress_set_text(&progress, "Loading...");
    zui_progress_set_value(&progress, 0);
    zui_progress_draw(&progress);
    
    // Status bar
    zui_status_bar("Press ↑/↓ to navigate, Enter to select, q to quit", ZUI_GRAY);
    
    bool running = true;
    char key;
    int progress_val = 0;
    
    while (running) {
        key = zui_get_key();
        
        if (key == 'q' || key == 'Q') {
            running = false;
            break;
        }
        
        switch (key) {
            case 'A': // Up arrow (ESC[A)
                zui_menu_navigate(&menu, -1);
                zui_menu_draw(&menu);
                break;
            case 'B': // Down arrow
                zui_menu_navigate(&menu, 1);
                zui_menu_draw(&menu);
                break;
            case '\n':
            case '\r': // Enter
                if (menu.selected == 3) { // Exit
                    running = false;
                } else {
                    // Update status
                    char status[100];
                    snprintf(status, sizeof(status), "Selected: %s", items[menu.selected]);
                    zui_status_bar(status, ZUI_GREEN);
                }
                break;
            default:
                break;
        }
        
        // Animate progress
        if (key != 0) {
            progress_val = (progress_val + 5) % 100;
            zui_progress_set_value(&progress, progress_val);
            zui_progress_draw(&progress);
        }
    }
    
    // Cleanup
    zui_label_free(&title);
    zui_progress_free(&progress);
    zui_clear();
    printf("Goodbye!\n");
    
    return 0;
}
