// color_lib.c
#include <stdio.h>
#include "../src/lib/color.h" // atau nama file header Anda

int main() {
    // Auto-detect terminal color support
    color_auto_detect();
    
    // Basic colored text
    printf("=== Basic Colors ===\n");
    print_colored("Hello in Red!\n", MATERIAL_RED);
    print_colored("Hello in Green!\n", MATERIAL_GREEN);
    print_colored("Hello in Blue!\n", MATERIAL_BLUE);
    
    // Text with background
    printf("\n=== Background Colors ===\n");
    print_with_bg("White on Blue", rgb_create(255, 255, 255), MATERIAL_BLUE);
    printf("\n");
    
    // Styled text
    printf("\n=== Styled Text ===\n");
    Style my_style = style_create(MATERIAL_ORANGE, DRACULA_BG);
    style_set_bold(&my_style, 1);
    style_set_underline(&my_style, 1);
    print_styled("Bold & Underlined Orange Text\n", &my_style);
    
    // Gradient effects
    printf("\n=== Gradient Effects ===\n");
    print_gradient("Gradient from Red to Blue\n", MATERIAL_RED, MATERIAL_BLUE);
    print_rainbow("Rainbow Text Effect!\n");
    
    // Formatted output
    printf("\n=== Formatted Output ===\n");
    Style info_style = style_create(MATERIAL_CYAN, DRACULA_BG);
    printf_styled(&info_style, "Temperature: %.1f°C | Status: %s\n", 36.5, "OK");
    
    // Error/Success messages (sekarang menggunakan fungsi bukan macro)
    printf("\n=== Message Types ===\n");
    print_error("Error: File not found!\n");
    print_success("Success: Operation completed\n");
    print_warning("Warning: Low disk space\n");
    print_info("Info: Update available\n");
    
    // Formatted error/success
    printf_error("Error code: %d - %s\n", 404, "Not Found");
    printf_success("Completed: %d%%\n", 100);
    
    // Custom hex color
    printf("\n=== Custom Colors ===\n");
    RGB custom = rgb_from_hex("#FF6B6B");
    print_colored("Custom Hex Color\n", custom);
    
    // Color manipulation
    printf("\n=== Color Manipulation ===\n");
    RGB base = MATERIAL_PURPLE;
    print_colored("Original", base);
    printf(" -> ");
    print_colored("Lightened", rgb_lighten(base, 0.5));
    printf(" -> ");
    print_colored("Darkened", rgb_darken(base, 0.5));
    printf("\n");
    
    return 0;
}
