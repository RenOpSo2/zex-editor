ZUI Library API Documentation

Daftar Isi

1. Setup & Initialization
2. Screen & Display
3. Label
4. Menu
5. Progress Bar
6. Button
7. Input
8. Box/Window
9. Status Bar
10. Keyboard Input
11. Color & Style Constants

---

Setup & Initialization

zui_init()

Initialize ZUI library. Must be called before using any other functions.

```c
void zui_init(void);
```

Example:

```c
zui_init();
```

zui_enable_raw_mode() & zui_disable_raw_mode()

Enable/disable raw terminal mode manually (auto-handled by zui_init()).

```c
void zui_enable_raw_mode(void);
void zui_disable_raw_mode(void);
```

---

Screen & Display

zui_clear()

Clear entire screen and move cursor to home position.

```c
void zui_clear(void);
```

zui_refresh()

Flush output buffer to screen.

```c
void zui_refresh(void);
```

zui_reset_style()

Reset all text styles (color, bold, underline, etc.).

```c
void zui_reset_style(void);
```

zui_get_size()

Get terminal size in rows and columns.

```c
ZuiSize zui_get_size(void);
```

Types:

```c
typedef struct {
    int rows;  // Number of rows
    int cols;  // Number of columns
} ZuiSize;
```

Example:

```c
ZuiSize size = zui_get_size();
printf("Terminal: %dx%d\n", size.cols, size.rows);
```

---

Label

zui_label_new()

Create a new label.

```c
ZuiLabel zui_label_new(const char *text, int x, int y);
```

Parameters:

· text: Label text
· x: Column position (1-based)
· y: Row position (1-based)

Example:

```c
ZuiLabel label = zui_label_new("Hello World", 10, 5);
```

zui_label_set_color()

Set text color.

```c
void zui_label_set_color(ZuiLabel *label, const char *color);
```

zui_label_set_bg()

Set background color.

```c
void zui_label_set_bg(ZuiLabel *label, const char *bg);
```

zui_label_set_bold()

Enable/disable bold text.

```c
void zui_label_set_bold(ZuiLabel *label, bool bold);
```

zui_label_draw()

Draw label on screen.

```c
void zui_label_draw(ZuiLabel *label);
```

zui_label_free()

Free label resources.

```c
void zui_label_free(ZuiLabel *label);
```

Complete Example:

```c
ZuiLabel label = zui_label_new("Welcome!", 5, 3);
zui_label_set_color(&label, ZUI_CYAN);
zui_label_set_bg(&label, ZUI_BG_BLUE);
zui_label_set_bold(&label, true);
zui_label_draw(&label);
// ... later ...
zui_label_free(&label);
```

---

Menu

zui_menu_new()

Create a new menu with items.

```c
ZuiMenu zui_menu_new(char **items, int count, int x, int y);
```

Parameters:

· items: Array of strings
· count: Number of items
· x, y: Position

Example:

```c
char *options[] = {"Start", "Settings", "About", "Exit"};
ZuiMenu menu = zui_menu_new(options, 4, 10, 5);
```

zui_menu_draw()

Draw menu on screen.

```c
void zui_menu_draw(ZuiMenu *menu);
```

zui_menu_navigate()

Navigate menu items (up/down).

```c
void zui_menu_navigate(ZuiMenu *menu, int direction);
```

Parameters:

· direction: -1 for up, 1 for down

Example:

```c
// On arrow key press
if (key == 'A') zui_menu_navigate(&menu, -1);  // Up
if (key == 'B') zui_menu_navigate(&menu, 1);   // Down
zui_menu_draw(&menu);
```

zui_menu_free()

Free menu resources.

```c
void zui_menu_free(ZuiMenu *menu);
```

Menu Properties:

```c
menu.color = ZUI_WHITE;           // Normal item color
menu.bg_color = NULL;              // Normal background
menu.selected_color = ZUI_BLACK;   // Selected item color
menu.selected_bg = ZUI_BG_CYAN;    // Selected background
```

---

Progress Bar

zui_progress_new()

Create a new progress bar.

```c
ZuiProgress zui_progress_new(int x, int y, int width, int max);
```

Parameters:

· x, y: Position
· width: Bar width in characters
· max: Maximum value

Example:

```c
ZuiProgress bar = zui_progress_new(5, 10, 40, 100);
```

zui_progress_set_value()

Set current progress value.

```c
void zui_progress_set_value(ZuiProgress *bar, int value);
```

zui_progress_set_text()

Set label text next to progress bar.

```c
void zui_progress_set_text(ZuiProgress *bar, const char *text);
```

zui_progress_draw()

Draw progress bar.

```c
void zui_progress_draw(ZuiProgress *bar);
```

zui_progress_free()

Free progress bar resources.

```c
void zui_progress_free(ZuiProgress *bar);
```

Complete Example:

```c
ZuiProgress progress = zui_progress_new(5, 10, 30, 100);
zui_progress_set_text(&progress, "Loading...");
zui_progress_set_value(&progress, 75);
zui_progress_draw(&progress);
// ... later ...
zui_progress_free(&progress);
```

Properties:

```c
progress.color = ZUI_GREEN;    // Bar color
progress.bg_color = ZUI_BG_WHITE; // Background
```

---

Button

zui_button_new()

Create a new button.

```c
ZuiButton zui_button_new(const char *text, int x, int y);
```

Example:

```c
ZuiButton btn = zui_button_new("Click Me", 10, 15);
```

zui_button_draw()

Draw button.

```c
void zui_button_draw(ZuiButton *button);
```

zui_button_set_color()

Set button colors.

```c
void zui_button_set_color(ZuiButton *button, const char *fg, const char *bg);
```

zui_button_free()

Free button resources.

```c
void zui_button_free(ZuiButton *button);
```

Properties:

```c
button.visible = true;  // Show/hide button
```

Example:

```c
ZuiButton btn = zui_button_new("OK", 20, 10);
zui_button_set_color(&btn, ZUI_WHITE, ZUI_BG_GREEN);
zui_button_draw(&btn);
```

---

Input

zui_input_new()

Create a new input field.

```c
ZuiInput zui_input_new(const char *prompt, int x, int y, int size);
```

Parameters:

· prompt: Label text before input
· x, y: Position
· size: Maximum input buffer size

Example:

```c
ZuiInput input = zui_input_new("Name", 5, 8, 30);
```

zui_input_draw()

Draw input field with cursor.

```c
void zui_input_draw(ZuiInput *input);
```

zui_input_handle_char()

Process a single character input.

```c
bool zui_input_handle_char(ZuiInput *input, char c);
```

Returns: true if Enter pressed, false otherwise

Example:

```c
char key = zui_get_key();
if (zui_input_handle_char(&input, key)) {
    printf("Input: %s\n", input.buffer);
}
```

zui_input_free()

Free input resources.

```c
void zui_input_free(ZuiInput *input);
```

Properties:

```c
input.buffer;        // Current input text
input.cursor_pos;    // Cursor position
input.color = ZUI_WHITE;
input.bg_color = ZUI_BG_BLACK;
```

Complete Example:

```c
ZuiInput input = zui_input_new("Username", 5, 8, 20);
input.color = ZUI_GREEN;
zui_input_draw(&input);

char key;
while (1) {
    key = zui_get_key();
    if (zui_input_handle_char(&input, key)) {
        break;  // Enter pressed
    }
    zui_input_draw(&input);
}
printf("You entered: %s\n", input.buffer);
zui_input_free(&input);
```

---

Box/Window

zui_draw_box()

Draw a box/window with optional title.

```c
void zui_draw_box(int x, int y, int width, int height, const char *title);
```

Parameters:

· x, y: Top-left position
· width, height: Box dimensions
· title: Optional title (NULL for no title)

Example:

```c
zui_draw_box(2, 2, 50, 20, "Main Menu");
zui_draw_box(55, 2, 30, 10, NULL);
```

---

Status Bar

zui_status_bar()

Display a status bar at bottom of screen.

```c
void zui_status_bar(const char *text, const char *color);
```

Parameters:

· text: Status message
· color: Text color (or NULL for default)

Example:

```c
zui_status_bar("Press 'q' to quit", ZUI_GRAY);
zui_status_bar("Error: Invalid input!", ZUI_RED);
zui_status_bar("Success!", ZUI_GREEN);
```

---

Keyboard Input

zui_get_key()

Get a single keypress (non-blocking).

```c
char zui_get_key(void);
```

Returns: Character code or 0 if no key pressed

Key Codes:

· Regular characters: ASCII (e.g., 'a', '1', etc.)
· Arrow keys: 'A' (up), 'B' (down), 'C' (right), 'D' (left)
· Enter: '\n' or '\r'
· Escape sequences: Handled automatically

Example:

```c
while (1) {
    char key = zui_get_key();
    if (key == 'q') break;
    
    if (key == 'A') printf("Up arrow\n");
    else if (key == 'B') printf("Down arrow\n");
    else if (key >= 32 && key <= 126) {
        printf("Key: %c\n", key);
    }
}
```

---

Color & Style Constants

Foreground Colors

```c
ZUI_BLACK     // Black
ZUI_RED       // Red
ZUI_GREEN     // Green
ZUI_YELLOW    // Yellow
ZUI_BLUE      // Blue
ZUI_MAGENTA   // Magenta
ZUI_CYAN      // Cyan
ZUI_WHITE     // White
ZUI_GRAY      // Gray (bright black)
```

Background Colors

```c
ZUI_BG_BLACK    // Black background
ZUI_BG_RED      // Red background
ZUI_BG_GREEN    // Green background
ZUI_BG_YELLOW   // Yellow background
ZUI_BG_BLUE     // Blue background
ZUI_BG_MAGENTA  // Magenta background
ZUI_BG_CYAN     // Cyan background
ZUI_BG_WHITE    // White background
```

Text Styles

```c
ZUI_BOLD        // Bold text
ZUI_DIM         // Dim text
ZUI_UNDERLINE   // Underlined text
ZUI_BLINK       // Blinking text
ZUI_REVERSE     // Reverse colors
ZUI_HIDDEN      // Hidden text
ZUI_RESET       // Reset all styles
```

Example Combining Styles:

```c
printf("%s%s%sHello%s", ZUI_BOLD, ZUI_CYAN, ZUI_BG_BLUE, ZUI_RESET);
```

---

Complete Example Application

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "zui.h"

int main() {
    // Initialize
    zui_init();
    zui_clear();
    
    // Create UI components
    ZuiLabel title = zui_label_new("=== My App ===", 2, 2);
    zui_label_set_color(&title, ZUI_CYAN);
    zui_label_set_bold(&title, true);
    zui_label_draw(&title);
    
    char *items[] = {"Start", "Settings", "About", "Exit"};
    ZuiMenu menu = zui_menu_new(items, 4, 5, 5);
    menu.selected_bg = ZUI_BG_GREEN;
    zui_menu_draw(&menu);
    
    ZuiInput input = zui_input_new("Input", 5, 12, 30);
    zui_input_draw(&input);
    
    ZuiProgress bar = zui_progress_new(5, 15, 40, 100);
    zui_progress_set_text(&bar, "Progress");
    zui_progress_draw(&bar);
    
    zui_status_bar("↑↓ navigate | Enter select | q quit", ZUI_GRAY);
    
    // Main loop
    bool running = true;
    char key;
    int progress_val = 0;
    
    while (running) {
        key = zui_get_key();
        
        if (key == 'q' || key == 'Q') break;
        
        if (key == 'A' || key == 'B') {
            zui_menu_navigate(&menu, (key == 'A') ? -1 : 1);
            zui_menu_draw(&menu);
        }
        else if (key == '\n' || key == '\r') {
            if (menu.selected == 3) break;
            char msg[100];
            snprintf(msg, sizeof(msg), "Selected: %s", items[menu.selected]);
            zui_status_bar(msg, ZUI_GREEN);
        }
        else if (key >= 32 && key <= 126) {
            zui_input_handle_char(&input, key);
            zui_input_draw(&input);
        }
        
        // Animate progress
        progress_val = (progress_val + 1) % 100;
        zui_progress_set_value(&bar, progress_val);
        zui_progress_draw(&bar);
        
        usleep(50000);
    }
    
    // Cleanup
    zui_label_free(&title);
    zui_input_free(&input);
    zui_progress_free(&bar);
    zui_clear();
    
    printf("Goodbye!\n");
    return 0;
}
```

---

Best Practices

1. Always call zui_init() before using any UI functions
2. Check return values for memory allocation functions
3. Free resources with corresponding _free() functions
4. Handle signals for clean exit (auto-handled)
5. Use zui_get_size() for responsive layouts
6. Refresh after drawing with zui_refresh() or auto-flush
7. Reset styles after custom formatting

---

Common Pitfalls

· ❌ Forgetting to call zui_init()
· ❌ Not checking if key is 0 (no key pressed)
· ❌ Using positions outside terminal bounds
· ❌ Memory leaks from not calling _free()
· ❌ Mixing raw output with ZUI functions

---

Quick Reference Card

```c
// Setup
zui_init();
zui_clear();

// Components
ZuiLabel l = zui_label_new("Text", x, y);
zui_label_draw(&l);
zui_label_free(&l);

ZuiMenu m = zui_menu_new(items, count, x, y);
zui_menu_draw(&m);
zui_menu_navigate(&m, direction);

ZuiProgress p = zui_progress_new(x, y, width, max);
zui_progress_set_value(&p, value);
zui_progress_draw(&p);

ZuiInput i = zui_input_new("Prompt", x, y, size);
zui_input_draw(&i);
zui_input_handle_char(&i, key);

// Utilities
zui_draw_box(x, y, w, h, "Title");
zui_status_bar("Status", ZUI_GREEN);
char key = zui_get_key();
ZuiSize s = zui_get_size();

// Cleanup
zui_clear();
// Resources auto-freed on exit
```

---

Happy coding with ZUI! 🎨
