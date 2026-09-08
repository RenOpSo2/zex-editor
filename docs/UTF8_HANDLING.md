# UTF-8 Handling

Zex handles UTF-8 text encoding for international character support, including proper display width calculations for various character types (ASCII, CJK, emoji, combining characters).

## UTF-8 Encoding Overview

UTF-8 is a variable-width encoding that uses 1-4 bytes per character:

| Byte Range | Encoding | Bytes |
|------------|----------|-------|
| 0x00-0x7F | ASCII | 1 byte |
| 0x80-0x7FF | 2-byte sequence | 2 bytes |
| 0x800-0xFFFF | 3-byte sequence | 3 bytes |
| 0x10000-0x10FFFF | 4-byte sequence | 4 bytes |

## UTF-8 Decoding

The decoder is defensive - it handles malformed or truncated sequences gracefully by treating them as single bytes rather than crashing.

### Trail Byte Count

Determine how many continuation bytes follow the lead byte:

```c
int utf8_trail_count(unsigned char lead)
{
    if (lead < 0x80) return 0;                       // ASCII
    if ((lead & 0xE0) == 0xC0) return 1;            // 110xxxxx
    if ((lead & 0xF0) == 0xE0) return 2;            // 1110xxxx
    if ((lead & 0xF8) == 0xF0) return 3;            // 11110xxx
    return 0;                                       // Invalid lead
}
```

### UTF-8 Decode

Decode a UTF-8 sequence to a Unicode code point:

```c
size_t utf8_decode(const unsigned char* s, size_t avail, uint32_t* cp)
{
    if (cp) *cp = 0;
    if (!s || avail == 0) return 0;
    
    unsigned char b = s[0];
    int n = utf8_trail_count(b);
    
    // ASCII character
    if (n == 0) {
        if (cp) *cp = b;
        return 1;
    }
    
    // Not enough bytes - decode lead alone (defensive)
    if ((size_t)n + 1u > avail) {
        if (cp) *cp = b;
        return 1;
    }
    
    // Decode continuation bytes
    uint32_t code = (uint32_t)(b & (0x7Fu >> n));
    for (int i = 1; i <= n; i++) {
        unsigned char cb = s[i];
        if ((cb & 0xC0) != 0x80) {  // Invalid continuation
            if (cp) *cp = b;
            return 1;
        }
        code = (code << 6) | (cb & 0x3Fu);
    }
    
    // Reject overlong/out-of-range forms
    if (code > 0x10FFFFu) {
        if (cp) *cp = b;
        return 1;
    }
    
    if (cp) *cp = code;
    return (size_t)n + 1u;
}
```

### Usage Example

```c
unsigned char text[] = "Hello 世界";
uint32_t cp;
size_t consumed;

// Decode 'H' (ASCII)
size_t len = utf8_decode(text, 5, &cp);
// cp = 0x48 ('H'), len = 1

// Decode '世' (3-byte UTF-8)
size_t len = utf8_decode(text + 6, 3, &cp);
// cp = 0x4E16, len = 3
```

## Display Width Calculation

Different characters occupy different display widths:
- ASCII: 1 column
- CJK characters: 2 columns
- Emoji: 2 columns
- Combining characters: 0 columns (combine with previous)
- Control characters: 2 columns (displayed as ^X)

### Code Point Width

```c
uint32_t codepoint_width(uint32_t cp)
{
    // Control characters (handled elsewhere)
    if (cp < 0x20 || cp == 0x7F) return 0;
    
    // Zero-width / combining marks
    if (cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0x2060 ||
        cp == 0xFEFF) {
        return 0;
    }
    if ((cp >= 0x0300 && cp <= 0x036F) ||  // Combining diacritics
        (cp >= 0x1AB0 && cp <= 0x1AFF) ||
        (cp >= 0x1DC0 && cp <= 0x1DFF) ||
        (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0xFE00 && cp <= 0xFE0F)) {  // Variation selectors
        return 0;
    }
    
    // East-Asian Wide / Fullwidth / emoji: 2 columns
    if (cp >= 0x1100 && cp <= 0x115F) return 2;  // Hangul Jamo
    if (cp >= 0x2E80 && cp <= 0x303E) return 2;  // CJK radicals
    if (cp >= 0x3041 && cp <= 0x33FF) return 2;  // Hiragana..CJK
    if (cp >= 0x3400 && cp <= 0x4DBF) return 2;  // CJK Ext A
    if (cp >= 0x4E00 && cp <= 0x9FFF) return 2;  // CJK Unified
    if (cp >= 0xA000 && cp <= 0xA4CF) return 2;  // Yi
    if (cp >= 0xAC00 && cp <= 0xD7A3) return 2;  // Hangul Syllables
    if (cp >= 0xF900 && cp <= 0xFAFF) return 2;  // CJK Compatibility
    if (cp >= 0xFE30 && cp <= 0xFE4F) return 2;  // CJK Compat forms
    if (cp >= 0xFF00 && cp <= 0xFF60) return 2;  // Fullwidth forms
    if (cp >= 0xFFE0 && cp <= 0xFFE6) return 2;  // Fullwidth symbols
    if (cp >= 0x1F300 && cp <= 0x1FAFF) return 2;  // emoji
    if (cp >= 0x1F000 && cp <= 0x1FFFF) return 2;  // supplemental
    if (cp >= 0x20000 && cp <= 0x3FFFD) return 2;  // CJK Ext B+
    
    return 1;  // Default: 1 column
}
```

### Character Width with Context

Calculate width considering tab expansion and column position:

```c
uint32_t char_width(const unsigned char* s, size_t avail, 
                   uint32_t col, uint32_t tab_size, 
                   size_t* consumed)
{
    if (consumed) *consumed = 0;
    if (!s || avail == 0) return 0;
    
    unsigned char b = s[0];
    
    // Tab character
    if (b == '\t') {
        uint32_t ts = (tab_size > 0) ? tab_size : 1;
        if (consumed) *consumed = 1;
        return ts - (col % ts);  // Space to next tab stop
    }
    
    // Control character (displayed as ^X)
    if (b < 0x20 || b == 127) {
        if (consumed) *consumed = 1;
        return 2;  // Caret notation (^X)
    }
    
    // UTF-8 character
    uint32_t cp;
    size_t n = utf8_decode(s, avail, &cp);
    if (consumed) *consumed = n;
    return codepoint_width(cp);
}
```

### Usage Example

```c
unsigned char text[] = "A\t中😀";
uint32_t col = 0;
uint32_t tab_size = 4;
size_t consumed;

// 'A' at column 0
uint32_t w = char_width(text, 1, col, tab_size, &consumed);
// w = 1, consumed = 1, col = 1

// '\t' at column 1
w = char_width(text + 1, 1, col, tab_size, &consumed);
// w = 3 (space to column 4), consumed = 1, col = 4

// '中' at column 4
w = char_width(text + 2, 3, col, tab_size, &consumed);
// w = 2 (CJK), consumed = 3, col = 6

// '😀' at column 6
w = char_width(text + 5, 4, col, tab_size, &consumed);
// w = 2 (emoji), consumed = 4, col = 8
```

## Tab Expansion

Tabs are expanded to spaces based on column position and tab size.

### Tab Stop Calculation

```c
// Tab at column 0 with tab_size 4 → spaces to column 4 (3 spaces)
// Tab at column 2 with tab_size 4 → spaces to column 4 (2 spaces)
// Tab at column 4 with tab_size 4 → spaces to column 8 (4 spaces)

uint32_t tab_spaces = tab_size - (col % tab_size);
```

### Example

```
Column: 0123456789
Text:   A\tB
Width:  1 3 1
       ^^^^
       A   B
```

## Control Character Display

Control characters (0x00-0x1F, 0x7F) are displayed using caret notation:

| Character | Display | Width |
|-----------|---------|-------|
| 0x00 (NUL) | ^@ | 2 |
| 0x01 (SOH) | ^A | 2 |
| 0x1A (SUB) | ^Z | 2 |
| 0x1B (ESC) | ^[ | 2 |
| 0x7F (DEL) | ^? | 2 |

```c
if (b < 0x20 || b == 127) {
    char pair[2] = {'^', (char)(b == 127 ? '?' : (char)(b + '@'))};
    // Display as ^X
}
```

## Rendering with Width Calculation

When rendering text, track column position to handle tabs correctly:

```c
uint32_t col = 0;
uint32_t tab_size = 4;

for (size_t i = 0; i < len; ) {
    size_t consumed;
    uint32_t w = char_width(text + i, len - i, col, tab_size, &consumed);
    
    // Ensure we don't exceed screen width
    if (col + w > screen_width) break;
    
    // Output character
    output_char(text + i, consumed);
    
    col += w;
    i += consumed;
}
```

## Defensive Decoding

The decoder is designed to handle:
- **Truncated sequences** - Multibyte character split across buffer boundary
- **Invalid lead bytes** - Bytes that don't start valid UTF-8
- **Invalid continuation** - Bytes that should be continuation but aren't
- **Overlong encoding** - Non-shortest form encoding
- **Out of range** - Code points beyond Unicode range

All these cases are handled by treating the problematic byte as a single character, preventing crashes and allowing the editor to continue.

## Display Width in Editor Context

### Line Wrapping

When wrapping lines, use display width, not byte count:

```c
uint32_t col = 0;
for (size_t i = 0; i < len; ) {
    size_t consumed;
    uint32_t w = char_width(text + i, len - i, col, tab_size, &consumed);
    
    if (col + w > screen_width) {
        // Wrap to next line
        col = 0;
        continue;
    }
    
    col += w;
    i += consumed;
}
```

### Cursor Positioning

Convert between byte offset and display column:

```c
// Byte offset to column
uint32_t offset_to_column(const char* text, size_t target_offset)
{
    uint32_t col = 0;
    for (size_t i = 0; i < target_offset; ) {
        size_t consumed;
        uint32_t w = char_width((unsigned char*)text + i, 
                               target_offset - i, col, tab_size, &consumed);
        col += w;
        i += consumed;
    }
    return col;
}
```

## Testing UTF-8 Handling

```bash
# Test with various UTF-8 characters
./bin/zex test_utf8.txt

# Test file containing:
# - ASCII
# - CJK characters
# - Emoji
# - Combining characters
# - Tabs
# - Control characters
```

## Best Practices

1. **Always use utf8_decode** - Don't assume UTF-8 is valid
2. **Track column position** - Use col parameter for tab expansion
3. **Handle combining marks** - They have zero width
4. **Test with edge cases** - Truncated sequences, invalid UTF-8
5. **Use display width for layout** - Not byte count

## Limitations

- **Grapheme clusters** - Not fully supported (combining marks treated separately)
- **Bidirectional text** - Not supported (left-to-right only)
- **Complex scripts** - Arabic, Indic scripts may not render correctly
- **Font dependence** - Actual display depends on terminal font