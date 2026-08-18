TUI Color Library - API Documentation

📚 Daftar Isi

· Instalasi
· Inisialisasi
· Tipe Data
· Color Management
· Style Management
· Printing API
· Efek Khusus
· Palet Warna
· Contoh Lengkap

---

Instalasi

```c
// Cukup include header file
#include "tui_color.h"

// Compile dengan math library
// gcc program.c -o program -lm
```

---

Inisialisasi

color_auto_detect()

Mendeteksi otomatis dukungan warna terminal.

```c
void color_auto_detect(void);
```

Contoh:

```c
color_auto_detect(); // Auto-detect terminal support
```

color_set_mode()

Set mode warna secara manual.

```c
void color_set_mode(ColorMode mode);
```

Mode yang tersedia:

· COLOR_MODE_16 - 16 colors (basic)
· COLOR_MODE_256 - 256 colors
· COLOR_MODE_TRUECOLOR - 24-bit true color (16.7 juta warna)

Contoh:

```c
color_set_mode(COLOR_MODE_TRUECOLOR);
```

color_enable() / color_disable()

Enable/disable output warna.

```c
void color_enable(void);
void color_disable(void);
```

color_is_enabled()

Cek apakah warna diaktifkan.

```c
int color_is_enabled(void);
// Returns: 1 jika enabled, 0 jika disabled
```

---

Tipe Data

RGB

Struktur untuk warna RGB.

```c
typedef struct {
    int r, g, b;  // Range: 0-255
} RGB;
```

HSV

Struktur untuk warna HSV.

```c
typedef struct {
    int h;        // Hue: 0-360
    double s, v;  // Saturation & Value: 0.0-1.0
} HSV;
```

Style

Struktur untuk styling lengkap.

```c
typedef struct {
    RGB fg;            // Foreground color
    RGB bg;            // Background color
    int bold;          // 1 = bold
    int italic;        // 1 = italic
    int underline;     // 1 = underline
    int strikethrough; // 1 = strikethrough
    int reverse;       // 1 = reverse video
    int blink;         // 1 = blink
} Style;
```

---

Color Management

rgb_create()

Membuat warna RGB baru.

```c
RGB rgb_create(int r, int g, int b);
```

Contoh:

```c
RGB my_color = rgb_create(255, 100, 50);
```

rgb_from_hex()

Membuat warna dari string hex.

```c
RGB rgb_from_hex(const char *hex);
```

Contoh:

```c
RGB color1 = rgb_from_hex("#FF5733");  // Full hex
RGB color2 = rgb_from_hex("#F53");     // Short hex
RGB color3 = rgb_from_hex("FF5733");   // Tanpa '#'
```

rgb_mix()

Mencampur dua warna.

```c
RGB rgb_mix(RGB a, RGB b, float t);
// t = 0.0 → warna a
// t = 0.5 → campuran 50-50
// t = 1.0 → warna b
```

Contoh:

```c
RGB mixed = rgb_mix(RED, BLUE, 0.5); // Purple
```

rgb_lighten()

Mencerahkan warna.

```c
RGB rgb_lighten(RGB c, float amount);
// amount: 0.0 (tidak berubah) - 1.0 (putih)
```

Contoh:

```c
RGB light_red = rgb_lighten(MATERIAL_RED, 0.3);
```

rgb_darken()

Menggelapkan warna.

```c
RGB rgb_darken(RGB c, float amount);
// amount: 0.0 (tidak berubah) - 1.0 (hitam)
```

Contoh:

```c
RGB dark_blue = rgb_darken(MATERIAL_BLUE, 0.5);
```

rgb_to_hsv()

Konversi RGB ke HSV.

```c
HSV rgb_to_hsv(RGB rgb);
```

hsv_to_rgb()

Konversi HSV ke RGB.

```c
RGB hsv_to_rgb(HSV hsv);
```

Contoh:

```c
HSV hsv = {120, 1.0, 1.0};  // Pure green
RGB green = hsv_to_rgb(hsv);
```

---

Style Management

style_create()

Membuat style baru.

```c
Style style_create(RGB fg, RGB bg);
```

Contoh:

```c
Style my_style = style_create(MATERIAL_WHITE, MATERIAL_BLUE);
```

style_default()

Membuat style default (putih di atas hitam).

```c
Style style_default(void);
```

style_set_bold()

Set atribut bold.

```c
void style_set_bold(Style *s, int enable);
```

style_set_italic()

Set atribut italic.

```c
void style_set_italic(Style *s, int enable);
```

style_set_underline()

Set atribut underline.

```c
void style_set_underline(Style *s, int enable);
```

style_set_strikethrough()

Set atribut strikethrough.

```c
void style_set_strikethrough(Style *s, int enable);
```

Contoh lengkap:

```c
Style title_style = style_create(MATERIAL_YELLOW, DRACULA_BG);
style_set_bold(&title_style, 1);
style_set_underline(&title_style, 1);
style_set_italic(&title_style, 1);
```

---

Printing API

print_styled()

Print teks dengan style.

```c
void print_styled(const char *text, const Style *style);
```

Contoh:

```c
Style s = style_create(MATERIAL_RED, DRACULA_BG);
print_styled("Error message", &s);
```

println_styled()

Print teks dengan style + newline.

```c
void println_styled(const char *text, const Style *style);
```

print_colored()

Print teks dengan foreground color saja.

```c
void print_colored(const char *text, RGB fg);
```

Contoh:

```c
print_colored("Hello World", MATERIAL_GREEN);
```

print_with_bg()

Print teks dengan foreground dan background.

```c
void print_with_bg(const char *text, RGB fg, RGB bg);
```

Contoh:

```c
print_with_bg("Warning", MATERIAL_BLACK, MATERIAL_YELLOW);
```

printf_styled()

Print formatted string dengan style.

```c
void printf_styled(const Style *style, const char *format, ...);
```

Contoh:

```c
Style info = style_create(MATERIAL_CYAN, DRACULA_BG);
printf_styled(&info, "User: %s | Age: %d\n", "John", 25);
```

---

Efek Khusus

print_gradient()

Print teks dengan efek gradient (foreground).

```c
void print_gradient(const char *text, RGB from, RGB to);
```

Contoh:

```c
print_gradient("Gradient Text", MATERIAL_RED, MATERIAL_BLUE);
```

print_gradient_bg()

Print teks dengan efek gradient (background).

```c
void print_gradient_bg(const char *text, RGB from, RGB to);
```

Contoh:

```c
print_gradient_bg("Background Gradient", DRACULA_PURPLE, DRACULA_PINK);
```

print_rainbow()

Print teks dengan efek pelangi.

```c
void print_rainbow(const char *text);
```

Contoh:

```c
print_rainbow("🌈 Rainbow Text! 🌈");
```

---

Fungsi Convenience

Pre-defined Styles

```c
Style style_error(void);    // Red text on Dracula background
Style style_success(void);  // Green text on Dracula background
Style style_warning(void);  // Orange text on Dracula background
Style style_info(void);     // Blue text on Dracula background
```

Pre-defined Print Functions

```c
void print_error(const char *msg);
void print_success(const char *msg);
void print_warning(const char *msg);
void print_info(const char *msg);
```

Contoh:

```c
print_error("File not found!");
print_success("Operation completed!");
print_warning("Low battery!");
print_info("Update available!");
```

Formatted Versions

```c
void printf_error(const char *format, ...);
void printf_success(const char *format, ...);
void printf_warning(const char *format, ...);
void printf_info(const char *format, ...);
```

Contoh:

```c
printf_error("Error %d: %s\n", 404, "Not Found");
printf_success("Downloaded %d%% complete\n", 100);
printf_warning("Battery at %d%%\n", 15);
printf_info("Version %s available\n", "2.0.0");
```

---

Palet Warna

Material Design Colors

```c
// Semua tersedia sebagai konstanta RGB
MATERIAL_RED          // #F44336
MATERIAL_PINK         // #E91E63
MATERIAL_PURPLE       // #9C27B0
MATERIAL_DEEP_PURPLE  // #673AB7
MATERIAL_INDIGO       // #3F51B5
MATERIAL_BLUE         // #2196F3
MATERIAL_LIGHT_BLUE   // #03A9F4
MATERIAL_CYAN         // #00BCD4
MATERIAL_TEAL         // #009688
MATERIAL_GREEN        // #4CAF50
MATERIAL_LIGHT_GREEN  // #8BC34A
MATERIAL_LIME         // #CDDC39
MATERIAL_YELLOW       // #FFEB3B
MATERIAL_AMBER        // #FFC107
MATERIAL_ORANGE       // #FF9800
MATERIAL_DEEP_ORANGE  // #FF5722
MATERIAL_BROWN        // #795548
MATERIAL_GREY         // #9E9E9E
MATERIAL_BLUE_GREY    // #607D8B
```

Dracula Theme Colors

```c
DRACULA_BG      // #282A36 (Background)
DRACULA_FG      // #F8F8F2 (Foreground)
DRACULA_CYAN    // #8BE9FD
DRACULA_GREEN   // #50FA7B
DRACULA_ORANGE  // #FFB86C
DRACULA_PINK    // #FF79C6
DRACULA_PURPLE  // #BD93F9
DRACULA_RED     // #FF5555
DRACULA_YELLOW  // #F1FA8C
```

Solarized Colors

```c
SOLARIZED_BASE03   // #002B36
SOLARIZED_BASE02   // #073642
SOLARIZED_BASE01   // #586E75
SOLARIZED_BASE00   // #657B83
SOLARIZED_BASE0    // #839496
SOLARIZED_BASE1    // #93A1A1
SOLARIZED_BASE2    // #EEE8D5
SOLARIZED_BASE3    // #FDF6E3
SOLARIZED_YELLOW   // #B58900
SOLARIZED_ORANGE   // #CB4B16
SOLARIZED_RED      // #DC322F
SOLARIZED_MAGENTA  // #D33682
SOLARIZED_VIOLET   // #6C71C4
SOLARIZED_BLUE     // #268BD2
SOLARIZED_CYAN     // #2AA198
SOLARIZED_GREEN    // #859900
```

---

Contoh Lengkap

Contoh 1: Basic Usage

```c
#include "tui_color.h"

int main() {
    color_auto_detect();
    
    print_colored("Hello World\n", MATERIAL_BLUE);
    print_with_bg("Warning!", MATERIAL_BLACK, MATERIAL_YELLOW);
    
    return 0;
}
```

Contoh 2: Styled Text

```c
#include "tui_color.h"

int main() {
    color_auto_detect();
    
    // Buat style custom
    Style title = style_create(MATERIAL_WHITE, DRACULA_PURPLE);
    style_set_bold(&title, 1);
    style_set_underline(&title, 1);
    
    print_styled("=== MY APP ===\n", &title);
    
    // Info message
    printf_info("Loading... %d%%\n", 75);
    
    // Success message
    printf_success("Loaded %d items in %.2f seconds\n", 100, 2.5);
    
    return 0;
}
```

Contoh 3: Color Manipulation

```c
#include "tui_color.h"

int main() {
    color_auto_detect();
    
    // Base color
    RGB base = MATERIAL_BLUE;
    
    // Create variations
    RGB light = rgb_lighten(base, 0.3);
    RGB dark = rgb_darken(base, 0.3);
    RGB mixed = rgb_mix(base, MATERIAL_RED, 0.5);
    
    // Display
    print_colored("Dark: ", dark);
    print_colored("Base: ", base);
    print_colored("Light: ", light);
    print_colored("Mixed: ", mixed);
    
    return 0;
}
```

Contoh 4: Gradient Effects

```c
#include "tui_color.h"

int main() {
    color_auto_detect();
    
    // Gradient text
    print_gradient("Fade from red to blue\n", 
                   MATERIAL_RED, MATERIAL_BLUE);
    
    // Rainbow
    print_rainbow("Rainbow effect!\n");
    
    // Gradient background
    print_gradient_bg("Background gradient\n",
                      DRACULA_PURPLE, DRACULA_PINK);
    
    return 0;
}
```

Contoh 5: Complex UI

```c
#include "tui_color.h"

int main() {
    color_auto_detect();
    
    // Header
    Style header = style_create(MATERIAL_WHITE, MATERIAL_INDIGO);
    style_set_bold(&header, 1);
    print_styled("  MY APPLICATION  \n", &header);
    
    // Menu items
    Style menu = style_create(MATERIAL_GREY, DRACULA_BG);
    print_styled("  1. New File\n", &menu);
    print_styled("  2. Open File\n", &menu);
    print_styled("  3. Save File\n", &menu);
    
    // Status bar
    Style status = style_create(MATERIAL_BLACK, MATERIAL_GREEN);
    printf_styled(&status, " Ready | Line: %d | Col: %d ", 10, 20);
    
    return 0;
}
```

---

Tips & Trik

1. Buat Style Reusable

```c
// Definisikan di awal program
Style error_style;
Style success_style;

void init_styles() {
    error_style = style_create(MATERIAL_RED, DRACULA_BG);
    style_set_bold(&error_style, 1);
    
    success_style = style_create(MATERIAL_GREEN, DRACULA_BG);
    style_set_bold(&success_style, 1);
}

// Gunakan berulang kali
print_styled("Error 1\n", &error_style);
print_styled("Error 2\n", &error_style);
```

2. Custom Color Palette

```c
// Buat palette sendiri
RGB MY_BRAND_COLOR = rgb_from_hex("#FF6B6B");
RGB MY_ACCENT = rgb_from_hex("#4ECDC4");
RGB MY_WARNING = rgb_from_hex("#FFE66D");

// Gunakan
print_colored("Brand Text", MY_BRAND_COLOR);
```

3. Nested Styles

```c
// Print multiple styles in one line
print_colored("[", MATERIAL_GREY);
print_colored("OK", MATERIAL_GREEN);
print_colored("] ", MATERIAL_GREY);
print_colored("Operation successful\n", MATERIAL_WHITE);
```

4. Dynamic Color

```c
// Warna berdasarkan kondisi
RGB status_color;
if (battery > 50) {
    status_color = MATERIAL_GREEN;
} else if (battery > 20) {
    status_color = MATERIAL_YELLOW;
} else {
    status_color = MATERIAL_RED;
}
print_colored("Battery Status", status_color);
```

---

Error Handling

Library ini tidak melempar error, tapi ada beberapa hal yang perlu diperhatikan:

1. Range RGB: Selalu gunakan nilai 0-255
2. Null pointer: Jangan pass NULL ke fungsi yang membutuhkan pointer
3. Terminal support: Selalu panggil color_auto_detect() di awal

---

Performance Tips

1. Reuse Style objects untuk performa lebih baik
2. Batch output jika memungkinkan
3. Disable color jika output di-redirect ke file:

```c
if (!isatty(STDOUT_FILENO)) {
    color_disable();
}
```

---

Kompatibilitas

Terminal 16 Colors 256 Colors True Color
Linux Terminal ✅ ✅ ✅
macOS Terminal ✅ ✅ ❌
iTerm2 ✅ ✅ ✅
Windows Terminal ✅ ✅ ✅
VS Code Terminal ✅ ✅ ✅
tmux ✅ ✅ ✅*

*Perlu konfigurasi tambahan untuk true color di tmux

---

Lisensi

MIT License - Bebas digunakan untuk proyek komersial maupun open source.

---

Happy Coding! 🎨
