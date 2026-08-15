// zui.h
#ifndef ZUI_H
#define ZUI_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdbool.h>

/* ==================== ANSI ESCAPE CODES ==================== */
#define ZUI_CLEAR       "\033[2J"
#define ZUI_HOME        "\033[H"
#define ZUI_RESET       "\033[0m"
#define ZUI_BOLD        "\033[1m"
#define ZUI_DIM         "\033[2m"
#define ZUI_UNDERLINE   "\033[4m"
#define ZUI_BLINK       "\033[5m"
#define ZUI_REVERSE     "\033[7m"
#define ZUI_HIDDEN      "\033[8m"

/* Foreground Colors */
#define ZUI_BLACK       "\033[30m"
#define ZUI_RED         "\033[31m"
#define ZUI_GREEN       "\033[32m"
#define ZUI_YELLOW      "\033[33m"
#define ZUI_BLUE        "\033[34m"
#define ZUI_MAGENTA     "\033[35m"
#define ZUI_CYAN        "\033[36m"
#define ZUI_WHITE       "\033[37m"
#define ZUI_GRAY        "\033[90m"

/* Background Colors */
#define ZUI_BG_BLACK    "\033[40m"
#define ZUI_BG_RED      "\033[41m"
#define ZUI_BG_GREEN    "\033[42m"
#define ZUI_BG_YELLOW   "\033[43m"
#define ZUI_BG_BLUE     "\033[44m"
#define ZUI_BG_MAGENTA  "\033[45m"
#define ZUI_BG_CYAN     "\033[46m"
#define ZUI_BG_WHITE    "\033[47m"

/* ==================== TYPES ==================== */
typedef struct {
    int rows;
    int cols;
} ZuiSize;

typedef struct {
    int x;
    int y;
} ZuiPos;

typedef struct {
    char *text;
    int x;
    int y;
    char *color;
    char *bg_color;
    bool bold;
    bool underline;
} ZuiLabel;

typedef struct {
    char **items;
    int count;
    int selected;
    int x;
    int y;
    int width;
    int height;
    char *color;
    char *bg_color;
    char *selected_color;
    char *selected_bg;
} ZuiMenu;

typedef struct {
    char *text;
    int x;
    int y;
    int width;
    int height;
    char *color;
    char *bg_color;
    int progress;
    int max;
} ZuiProgress;

typedef struct {
    char *text;
    int x;
    int y;
    char *color;
    char *bg_color;
    bool visible;
} ZuiButton;

typedef struct {
    char *prompt;
    char *buffer;
    int size;
    int x;
    int y;
    char *color;
    char *bg_color;
    int cursor_pos;
} ZuiInput;

/* ==================== INTERNAL STATE ==================== */
static struct termios zui_orig_termios;
static bool zui_raw_mode = false;
static bool zui_initialized = false;

/* ==================== INTERNAL FUNCTIONS ==================== */
static void zui_die(const char *msg) {
    perror(msg);
    exit(1);
}

static void zui_enable_raw_mode() {
    if (zui_raw_mode) return;
    
    if (tcgetattr(STDIN_FILENO, &zui_orig_termios) == -1)
        zui_die("tcgetattr");
    
    struct termios raw = zui_orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        zui_die("tcsetattr");
    
    zui_raw_mode = true;
}

static void zui_disable_raw_mode() {
    if (!zui_raw_mode) return;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &zui_orig_termios) == -1)
        zui_die("tcsetattr");
    zui_raw_mode = false;
}

static void zui_handle_sigint(int sig) {
    (void)sig;
    zui_disable_raw_mode();
    printf(ZUI_CLEAR ZUI_HOME);
    exit(0);
}

static void zui_init() {
    if (zui_initialized) return;
    
    signal(SIGINT, zui_handle_sigint);
    atexit(zui_disable_raw_mode);
    zui_enable_raw_mode();
    zui_initialized = true;
    
    printf(ZUI_CLEAR ZUI_HOME);
    fflush(stdout);
}

static ZuiSize zui_get_size() {
    struct winsize ws;
    ZuiSize size = {24, 80};
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1) {
        size.rows = ws.ws_row;
        size.cols = ws.ws_col;
    }
    return size;
}

static void zui_goto(int x, int y) {
    printf("\033[%d;%dH", y, x);
    fflush(stdout);
}

static void zui_clear_line() {
    printf("\033[2K");
    fflush(stdout);
}

static void zui_set_color(const char *fg, const char *bg) {
    if (fg) printf("%s", fg);
    if (bg) printf("%s", bg);
}

/* ==================== HIGH-LEVEL API ==================== */

/* -------- Screen -------- */
static inline void zui_clear() {
    printf(ZUI_CLEAR ZUI_HOME);
    fflush(stdout);
}

static inline void zui_refresh() {
    fflush(stdout);
}

static inline void zui_reset_style() {
    printf(ZUI_RESET);
    fflush(stdout);
}

/* -------- Label -------- */
static inline ZuiLabel zui_label_new(const char *text, int x, int y) {
    return (ZuiLabel){
        .text = strdup(text),
        .x = x,
        .y = y,
        .color = NULL,
        .bg_color = NULL,
        .bold = false,
        .underline = false
    };
}

static inline void zui_label_set_color(ZuiLabel *label, const char *color) {
    label->color = (char*)color;
}

static inline void zui_label_set_bg(ZuiLabel *label, const char *bg) {
    label->bg_color = (char*)bg;
}

static inline void zui_label_set_bold(ZuiLabel *label, bool bold) {
    label->bold = bold;
}

static inline void zui_label_draw(ZuiLabel *label) {
    zui_goto(label->x, label->y);
    if (label->bold) printf(ZUI_BOLD);
    if (label->underline) printf(ZUI_UNDERLINE);
    if (label->color) printf("%s", label->color);
    if (label->bg_color) printf("%s", label->bg_color);
    printf("%s", label->text);
    printf(ZUI_RESET);
    zui_refresh();
}

static inline void zui_label_free(ZuiLabel *label) {
    free(label->text);
}

/* -------- Menu -------- */
static inline ZuiMenu zui_menu_new(char **items, int count, int x, int y) {
    ZuiMenu menu = {
        .items = items,
        .count = count,
        .selected = 0,
        .x = x,
        .y = y,
        .width = 0,
        .height = count,
        .color = ZUI_WHITE,
        .bg_color = NULL,
        .selected_color = ZUI_BLACK,
        .selected_bg = ZUI_BG_WHITE
    };
    
    // Calculate max width
    for (int i = 0; i < count; i++) {
        int len = strlen(items[i]);
        if (len > menu.width) menu.width = len;
    }
    menu.width += 4; // padding
    
    return menu;
}

static inline void zui_menu_draw(ZuiMenu *menu) {
    for (int i = 0; i < menu->count; i++) {
        zui_goto(menu->x, menu->y + i);
        zui_clear_line();
        
        if (i == menu->selected) {
            if (menu->selected_color) printf("%s", menu->selected_color);
            if (menu->selected_bg) printf("%s", menu->selected_bg);
            printf(ZUI_BOLD);
        } else {
            if (menu->color) printf("%s", menu->color);
            if (menu->bg_color) printf("%s", menu->bg_color);
        }
        
        printf(" %-*s ", menu->width - 2, menu->items[i]);
        printf(ZUI_RESET);
    }
    zui_refresh();
}

static inline void zui_menu_navigate(ZuiMenu *menu, int direction) {
    menu->selected += direction;
    if (menu->selected < 0) menu->selected = menu->count - 1;
    if (menu->selected >= menu->count) menu->selected = 0;
}

static inline void zui_menu_free(ZuiMenu *menu) {
    // Don't free items, they're owned by caller
}

/* -------- Progress Bar -------- */
static inline ZuiProgress zui_progress_new(int x, int y, int width, int max) {
    return (ZuiProgress){
        .text = NULL,
        .x = x,
        .y = y,
        .width = width,
        .height = 1,
        .color = ZUI_GREEN,
        .bg_color = ZUI_BG_WHITE,
        .progress = 0,
        .max = max
    };
}

static inline void zui_progress_set_text(ZuiProgress *bar, const char *text) {
    if (bar->text) free(bar->text);
    bar->text = strdup(text);
}

static inline void zui_progress_set_value(ZuiProgress *bar, int value) {
    bar->progress = value;
    if (bar->progress > bar->max) bar->progress = bar->max;
    if (bar->progress < 0) bar->progress = 0;
}

static inline void zui_progress_draw(ZuiProgress *bar) {
    zui_goto(bar->x, bar->y);
    zui_clear_line();
    
    float percent = (float)bar->progress / bar->max;
    int filled = (int)(percent * bar->width);
    
    printf("%s", bar->color);
    printf(ZUI_BOLD);
    printf("[");
    
    for (int i = 0; i < bar->width; i++) {
        if (i < filled) {
            printf("#");
        } else {
            printf(" ");
        }
    }
    
    printf("]");
    printf(ZUI_RESET);
    
    if (bar->text) {
        printf(" %s", bar->text);
    }
    
    zui_refresh();
}

static inline void zui_progress_free(ZuiProgress *bar) {
    if (bar->text) free(bar->text);
}

/* -------- Button -------- */
static inline ZuiButton zui_button_new(const char *text, int x, int y) {
    return (ZuiButton){
        .text = strdup(text),
        .x = x,
        .y = y,
        .color = ZUI_WHITE,
        .bg_color = ZUI_BG_BLUE,
        .visible = true
    };
}

static inline void zui_button_draw(ZuiButton *button) {
    if (!button->visible) return;
    
    zui_goto(button->x, button->y);
    if (button->color) printf("%s", button->color);
    if (button->bg_color) printf("%s", button->bg_color);
    printf("[ %s ]", button->text);
    printf(ZUI_RESET);
    zui_refresh();
}

static inline void zui_button_set_color(ZuiButton *button, const char *fg, const char *bg) {
    button->color = (char*)fg;
    button->bg_color = (char*)bg;
}

static inline void zui_button_free(ZuiButton *button) {
    free(button->text);
}

/* -------- Input -------- */
static inline ZuiInput zui_input_new(const char *prompt, int x, int y, int size) {
    ZuiInput input = {
        .prompt = strdup(prompt),
        .buffer = calloc(size, sizeof(char)),
        .size = size,
        .x = x,
        .y = y,
        .color = ZUI_WHITE,
        .bg_color = ZUI_BG_BLACK,
        .cursor_pos = 0
    };
    return input;
}

static inline void zui_input_draw(ZuiInput *input) {
    zui_goto(input->x, input->y);
    zui_clear_line();
    
    printf("%s", input->color);
    printf("%s: ", input->prompt);
    printf(ZUI_RESET);
    
    if (input->bg_color) printf("%s", input->bg_color);
    printf("%s", input->buffer);
    printf(ZUI_RESET);
    
    // Draw cursor
    int cursor_x = input->x + strlen(input->prompt) + 2 + input->cursor_pos;
    zui_goto(cursor_x, input->y);
    printf(ZUI_REVERSE " " ZUI_RESET);
    zui_goto(cursor_x, input->y);
    
    zui_refresh();
}

static inline bool zui_input_handle_char(ZuiInput *input, char c) {
    if (c == '\n' || c == '\r') return true; // Enter pressed
    
    if (c == 127 || c == '\b') { // Backspace
        if (input->cursor_pos > 0) {
            input->cursor_pos--;
            input->buffer[input->cursor_pos] = '\0';
        }
        return false;
    }
    
    if (c >= 32 && c <= 126) { // Printable
        if (input->cursor_pos < input->size - 1) {
            input->buffer[input->cursor_pos] = c;
            input->cursor_pos++;
            input->buffer[input->cursor_pos] = '\0';
        }
    }
    
    return false;
}

static inline void zui_input_free(ZuiInput *input) {
    free(input->prompt);
    free(input->buffer);
}

/* -------- Window/Box -------- */
static inline void zui_draw_box(int x, int y, int width, int height, const char *title) {
    // Top border
    zui_goto(x, y);
    printf("┌");
    for (int i = 0; i < width - 2; i++) printf("─");
    printf("┐");
    
    // Title
    if (title) {
        zui_goto(x + 2, y);
        printf("%s", title);
    }
    
    // Middle
    for (int i = 1; i < height - 1; i++) {
        zui_goto(x, y + i);
        printf("│");
        zui_goto(x + width - 1, y + i);
        printf("│");
    }
    
    // Bottom border
    zui_goto(x, y + height - 1);
    printf("└");
    for (int i = 0; i < width - 2; i++) printf("─");
    printf("┘");
    
    zui_refresh();
}

/* -------- Key Input -------- */
static inline char zui_get_key() {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        return c;
    }
    return 0;
}

/* -------- Status Bar -------- */
static inline void zui_status_bar(const char *text, const char *color) {
    ZuiSize size = zui_get_size();
    zui_goto(1, size.rows);
    zui_clear_line();
    
    if (color) printf("%s", color);
    printf(" %s", text);
    printf(ZUI_RESET);
    zui_refresh();
}

#endif // ZUI_H
