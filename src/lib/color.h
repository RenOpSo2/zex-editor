/*
 * MODERN COLOR ABSTRACTION LIBRARY FOR TUI
 * Header-only | High-level API | Full color support
 * 
 */

#ifndef COLOR_H
#define COLOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

/* ============================================
 * COLOR TYPE DEFINITIONS
 * ============================================ */

typedef enum {
    COLOR_MODE_16,      // 16 colors (basic)
    COLOR_MODE_256,     // 256 colors
    COLOR_MODE_TRUECOLOR // 24-bit true color
} ColorMode;

typedef struct {
    int r, g, b;
} RGB;

typedef struct {
    int h;
    double s, v;
} HSV;

typedef struct {
    RGB fg;
    RGB bg;
    int bold;
    int italic;
    int underline;
    int strikethrough;
    int reverse;
    int blink;
} Style;

/* ============================================
 * COLOR PALETTES - MODERN & CURATED
 * ============================================ */

// Material Design Colors
static const RGB MATERIAL_RED = {244, 67, 54};
static const RGB MATERIAL_PINK = {233, 30, 99};
static const RGB MATERIAL_PURPLE = {156, 39, 176};
static const RGB MATERIAL_DEEP_PURPLE = {103, 58, 183};
static const RGB MATERIAL_INDIGO = {63, 81, 181};
static const RGB MATERIAL_BLUE = {33, 150, 243};
static const RGB MATERIAL_LIGHT_BLUE = {3, 169, 244};
static const RGB MATERIAL_CYAN = {0, 188, 212};
static const RGB MATERIAL_TEAL = {0, 150, 136};
static const RGB MATERIAL_GREEN = {76, 175, 80};
static const RGB MATERIAL_LIGHT_GREEN = {139, 195, 74};
static const RGB MATERIAL_LIME = {205, 220, 57};
static const RGB MATERIAL_YELLOW = {255, 235, 59};
static const RGB MATERIAL_AMBER = {255, 193, 7};
static const RGB MATERIAL_ORANGE = {255, 152, 0};
static const RGB MATERIAL_DEEP_ORANGE = {255, 87, 34};
static const RGB MATERIAL_BROWN = {121, 85, 72};
static const RGB MATERIAL_GREY = {158, 158, 158};
static const RGB MATERIAL_BLUE_GREY = {96, 125, 139};

// Dracula Theme
static const RGB DRACULA_BG = {40, 42, 54};
static const RGB DRACULA_FG = {248, 248, 242};
static const RGB DRACULA_CYAN = {139, 233, 253};
static const RGB DRACULA_GREEN = {80, 250, 123};
static const RGB DRACULA_ORANGE = {255, 184, 108};
static const RGB DRACULA_PINK = {255, 121, 198};
static const RGB DRACULA_PURPLE = {189, 147, 249};
static const RGB DRACULA_RED = {255, 85, 85};
static const RGB DRACULA_YELLOW = {241, 250, 140};

// Solarized
static const RGB SOLARIZED_BASE03 = {0, 43, 54};
static const RGB SOLARIZED_BASE02 = {7, 54, 66};
static const RGB SOLARIZED_BASE01 = {88, 110, 117};
static const RGB SOLARIZED_BASE00 = {101, 123, 131};
static const RGB SOLARIZED_BASE0 = {131, 148, 150};
static const RGB SOLARIZED_BASE1 = {147, 161, 161};
static const RGB SOLARIZED_BASE2 = {238, 232, 213};
static const RGB SOLARIZED_BASE3 = {253, 246, 227};
static const RGB SOLARIZED_YELLOW = {181, 137, 0};
static const RGB SOLARIZED_ORANGE = {203, 75, 22};
static const RGB SOLARIZED_RED = {220, 50, 47};
static const RGB SOLARIZED_MAGENTA = {211, 54, 130};
static const RGB SOLARIZED_VIOLET = {108, 113, 196};
static const RGB SOLARIZED_BLUE = {38, 139, 210};
static const RGB SOLARIZED_CYAN = {42, 161, 152};
static const RGB SOLARIZED_GREEN = {133, 153, 0};

/* ============================================
 * GLOBAL STATE
 * ============================================ */

static ColorMode current_color_mode = COLOR_MODE_TRUECOLOR;
static int color_enabled = 1;

/* ============================================
 * COLOR MODE MANAGEMENT
 * ============================================ */

static inline void color_set_mode(ColorMode mode) {
    current_color_mode = mode;
}

static inline ColorMode color_get_mode(void) {
    return current_color_mode;
}

static inline void color_enable(void) {
    color_enabled = 1;
}

static inline void color_disable(void) {
    color_enabled = 0;
}

static inline int color_is_enabled(void) {
    return color_enabled;
}

// Auto-detect terminal color support
static inline void color_auto_detect(void) {
    const char *term = getenv("TERM");
    const char *colorterm = getenv("COLORTERM");
    
    if (colorterm && (strstr(colorterm, "truecolor") || strstr(colorterm, "24bit"))) {
        current_color_mode = COLOR_MODE_TRUECOLOR;
    } else if (term && strstr(term, "256color")) {
        current_color_mode = COLOR_MODE_256;
    } else if (term && strstr(term, "color")) {
        current_color_mode = COLOR_MODE_16;
    } else {
        color_enabled = 0;
    }
}

/* ============================================
 * COLOR CONVERSION UTILITIES
 * ============================================ */

static inline RGB rgb_create(int r, int g, int b) {
    RGB color = {r, g, b};
    return color;
}

static inline RGB rgb_clamp(RGB c) {
    c.r = c.r < 0 ? 0 : (c.r > 255 ? 255 : c.r);
    c.g = c.g < 0 ? 0 : (c.g > 255 ? 255 : c.g);
    c.b = c.b < 0 ? 0 : (c.b > 255 ? 255 : c.b);
    return c;
}

static inline RGB rgb_mix(RGB a, RGB b, float t) {
    RGB result;
    result.r = (int)(a.r + (b.r - a.r) * t);
    result.g = (int)(a.g + (b.g - a.g) * t);
    result.b = (int)(a.b + (b.b - a.b) * t);
    return rgb_clamp(result);
}

static inline RGB rgb_lighten(RGB c, float amount) {
    return rgb_mix(c, rgb_create(255, 255, 255), amount);
}

static inline RGB rgb_darken(RGB c, float amount) {
    return rgb_mix(c, rgb_create(0, 0, 0), amount);
}

static inline RGB rgb_from_hex(const char *hex) {
    RGB color = {0, 0, 0};
    if (hex[0] == '#') hex++;
    
    int len = strlen(hex);
    if (len == 6) {
        sscanf(hex, "%02x%02x%02x", &color.r, &color.g, &color.b);
    } else if (len == 3) {
        char expanded[7];
        expanded[0] = hex[0]; expanded[1] = hex[0];
        expanded[2] = hex[1]; expanded[3] = hex[1];
        expanded[4] = hex[2]; expanded[5] = hex[2];
        expanded[6] = '\0';
        sscanf(expanded, "%02x%02x%02x", &color.r, &color.g, &color.b);
    }
    return color;
}

static inline HSV rgb_to_hsv(RGB rgb) {
    HSV hsv;
    double r = rgb.r / 255.0;
    double g = rgb.g / 255.0;
    double b = rgb.b / 255.0;
    
    double max = fmax(r, fmax(g, b));
    double min = fmin(r, fmin(g, b));
    double delta = max - min;
    
    hsv.v = max;
    hsv.s = (max > 0) ? (delta / max) : 0;
    
    if (delta == 0) {
        hsv.h = 0;
    } else if (max == r) {
        hsv.h = 60 * fmod(((g - b) / delta), 6);
    } else if (max == g) {
        hsv.h = 60 * (((b - r) / delta) + 2);
    } else {
        hsv.h = 60 * (((r - g) / delta) + 4);
    }
    
    if (hsv.h < 0) hsv.h += 360;
    
    return hsv;
}

static inline RGB hsv_to_rgb(HSV hsv) {
    RGB rgb;
    double c = hsv.v * hsv.s;
    double x = c * (1 - fabs(fmod(hsv.h / 60.0, 2) - 1));
    double m = hsv.v - c;
    
    double r, g, b;
    
    if (hsv.h < 60) {
        r = c; g = x; b = 0;
    } else if (hsv.h < 120) {
        r = x; g = c; b = 0;
    } else if (hsv.h < 180) {
        r = 0; g = c; b = x;
    } else if (hsv.h < 240) {
        r = 0; g = x; b = c;
    } else if (hsv.h < 300) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    rgb.r = (int)((r + m) * 255);
    rgb.g = (int)((g + m) * 255);
    rgb.b = (int)((b + m) * 255);
    
    return rgb;
}

/* ============================================
 * ANSI CODE GENERATION
 * ============================================ */

static inline int rgb_to_256(RGB c) {
    if (c.r == c.g && c.g == c.b) {
        if (c.r < 8) return 16;
        if (c.r > 248) return 231;
        return (int)(((c.r - 8) / 247.0) * 24) + 232;
    }
    
    int r = (int)((c.r / 255.0) * 5);
    int g = (int)((c.g / 255.0) * 5);
    int b = (int)((c.b / 255.0) * 5);
    
    return 16 + (36 * r) + (6 * g) + b;
}

static inline int rgb_to_16(RGB c) {
    int r = (c.r > 128) ? 1 : 0;
    int g = (c.g > 128) ? 1 : 0;
    int b = (c.b > 128) ? 1 : 0;
    
    if (r && g && b) return 15; // White
    if (!r && !g && !b) return 0; // Black
    
    // Bright colors
    int bright = (c.r > 192 || c.g > 192 || c.b > 192);
    
    if (r && !g && !b) return bright ? 9 : 1;   // Red
    if (!r && g && !b) return bright ? 10 : 2;  // Green
    if (!r && !g && b) return bright ? 12 : 4;  // Blue
    if (r && g && !b) return bright ? 11 : 3;   // Yellow
    if (r && !g && b) return bright ? 13 : 5;   // Magenta
    if (!r && g && b) return bright ? 14 : 6;   // Cyan
    
    return 7; // Grey
}

/* ============================================
 * STYLE MANAGEMENT
 * ============================================ */

static inline Style style_create(RGB fg, RGB bg) {
    Style s;
    s.fg = fg;
    s.bg = bg;
    s.bold = 0;
    s.italic = 0;
    s.underline = 0;
    s.strikethrough = 0;
    s.reverse = 0;
    s.blink = 0;
    return s;
}

static inline Style style_default(void) {
    return style_create(rgb_create(255, 255, 255), rgb_create(0, 0, 0));
}

static inline void style_set_bold(Style *s, int enable) {
    s->bold = enable;
}

static inline void style_set_italic(Style *s, int enable) {
    s->italic = enable;
}

static inline void style_set_underline(Style *s, int enable) {
    s->underline = enable;
}

static inline void style_set_strikethrough(Style *s, int enable) {
    s->strikethrough = enable;
}

/* ============================================
 * ANSI SEQUENCE BUILDER
 * ============================================ */

static inline const char* style_to_ansi(const Style *style) {
    static char buffer[256];
    char codes[128] = "";
    char temp[32];
    
    if (!color_enabled) {
        buffer[0] = '\0';
        return buffer;
    }
    
    // Text attributes
    if (style->bold) strcat(codes, "1;");
    if (style->italic) strcat(codes, "3;");
    if (style->underline) strcat(codes, "4;");
    if (style->blink) strcat(codes, "5;");
    if (style->reverse) strcat(codes, "7;");
    if (style->strikethrough) strcat(codes, "9;");
    
    // Foreground color
    switch (current_color_mode) {
        case COLOR_MODE_TRUECOLOR:
            sprintf(temp, "38;2;%d;%d;%d;", style->fg.r, style->fg.g, style->fg.b);
            strcat(codes, temp);
            break;
        case COLOR_MODE_256:
            sprintf(temp, "38;5;%d;", rgb_to_256(style->fg));
            strcat(codes, temp);
            break;
        case COLOR_MODE_16:
            sprintf(temp, "%d;", rgb_to_16(style->fg) + 30);
            strcat(codes, temp);
            break;
    }
    
    // Background color
    switch (current_color_mode) {
        case COLOR_MODE_TRUECOLOR:
            sprintf(temp, "48;2;%d;%d;%d;", style->bg.r, style->bg.g, style->bg.b);
            strcat(codes, temp);
            break;
        case COLOR_MODE_256:
            sprintf(temp, "48;5;%d;", rgb_to_256(style->bg));
            strcat(codes, temp);
            break;
        case COLOR_MODE_16:
            sprintf(temp, "%d;", rgb_to_16(style->bg) + 40);
            strcat(codes, temp);
            break;
    }
    
    // Remove trailing semicolon
    size_t len = strlen(codes);
    if (len > 0 && codes[len - 1] == ';') {
        codes[len - 1] = '\0';
    }
    
    sprintf(buffer, "\033[%sm", codes);
    return buffer;
}

static inline const char* style_reset(void) {
    return "\033[0m";
}

/* ============================================
 * HIGH-LEVEL PRINTING API
 * ============================================ */

// Basic styled print
static inline void print_styled(const char *text, const Style *style) {
    printf("%s%s%s", style_to_ansi(style), text, style_reset());
}

// Print with auto-reset
static inline void println_styled(const char *text, const Style *style) {
    printf("%s%s%s\n", style_to_ansi(style), text, style_reset());
}

// Print with foreground only
static inline void print_colored(const char *text, RGB fg) {
    Style s = style_create(fg, rgb_create(0, 0, 0));
    print_styled(text, &s);
}

// Print with foreground and background
static inline void print_with_bg(const char *text, RGB fg, RGB bg) {
    Style s = style_create(fg, bg);
    print_styled(text, &s);
}

// Print formatted string with style
static inline void printf_styled(const Style *style, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    printf("%s", style_to_ansi(style));
    vprintf(format, args);
    printf("%s", style_reset());
    
    va_end(args);
}

// Print gradient text (foreground only)
static inline void print_gradient(const char *text, RGB from, RGB to) {
    int len = strlen(text);
    if (len == 0) return;
    
    for (int i = 0; i < len; i++) {
        float t = (float)i / (len - 1);
        RGB color = rgb_mix(from, to, t);
        Style s = style_create(color, rgb_create(0, 0, 0));
        char ch[2] = {text[i], '\0'};
        print_styled(ch, &s);
    }
}

// Print text with gradient background
static inline void print_gradient_bg(const char *text, RGB from, RGB to) {
    int len = strlen(text);
    if (len == 0) return;
    
    for (int i = 0; i < len; i++) {
        float t = (float)i / (len - 1);
        RGB color = rgb_mix(from, to, t);
        Style s = style_create(rgb_create(255, 255, 255), color);
        char ch[2] = {text[i], '\0'};
        print_styled(ch, &s);
    }
}

// Rainbow text effect
static inline void print_rainbow(const char *text) {
    int len = strlen(text);
    if (len == 0) return;
    
    for (int i = 0; i < len; i++) {
        HSV hsv;
        hsv.h = (int)((float)i / len * 360) % 360;
        hsv.s = 1.0;
        hsv.v = 1.0;
        
        RGB color = hsv_to_rgb(hsv);
        Style s = style_create(color, rgb_create(0, 0, 0));
        char ch[2] = {text[i], '\0'};
        print_styled(ch, &s);
    }
}

/* ============================================
 * CONVENIENCE FUNCTIONS FOR COMMON STYLES
 * ============================================ */

// Pre-defined style functions
static inline Style style_error(void) {
    return style_create(MATERIAL_RED, DRACULA_BG);
}

static inline Style style_success(void) {
    return style_create(MATERIAL_GREEN, DRACULA_BG);
}

static inline Style style_warning(void) {
    return style_create(MATERIAL_ORANGE, DRACULA_BG);
}

static inline Style style_info(void) {
    return style_create(MATERIAL_BLUE, DRACULA_BG);
}

// Convenience print functions
static inline void print_error(const char *msg) {
    Style s = style_error();
    print_styled(msg, &s);
}

static inline void print_success(const char *msg) {
    Style s = style_success();
    print_styled(msg, &s);
}

static inline void print_warning(const char *msg) {
    Style s = style_warning();
    print_styled(msg, &s);
}

static inline void print_info(const char *msg) {
    Style s = style_info();
    print_styled(msg, &s);
}

// Formatted versions
static inline void printf_error(const char *format, ...) {
    Style s = style_error();
    va_list args;
    va_start(args, format);
    printf("%s", style_to_ansi(&s));
    vprintf(format, args);
    printf("%s", style_reset());
    va_end(args);
}

static inline void printf_success(const char *format, ...) {
    Style s = style_success();
    va_list args;
    va_start(args, format);
    printf("%s", style_to_ansi(&s));
    vprintf(format, args);
    printf("%s", style_reset());
    va_end(args);
}

static inline void printf_warning(const char *format, ...) {
    Style s = style_warning();
    va_list args;
    va_start(args, format);
    printf("%s", style_to_ansi(&s));
    vprintf(format, args);
    printf("%s", style_reset());
    va_end(args);
}

static inline void printf_info(const char *format, ...) {
    Style s = style_info();
    va_list args;
    va_start(args, format);
    printf("%s", style_to_ansi(&s));
    vprintf(format, args);
    printf("%s", style_reset());
    va_end(args);
}

#endif 
