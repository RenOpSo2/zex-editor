#include "dispwidth.h"

/*
 * Single, defensive implementation of "how wide is this character on screen".
 *
 * Everything is length-driven and falls back gracefully: a malformed or
 * truncated UTF-8 sequence is treated as a single raw byte rather than
 * causing an out-of-bounds read or a desynchronised decoder. This keeps the
 * editor robust against hostile or partial input (e.g. a multibyte
 * character split across a buffer page boundary).
 */

int utf8_trail_count(unsigned char lead)
{
    if (lead < 0x80) return 0;                       /* ASCII */
    if ((lead & 0xE0) == 0xC0) return 1;            /* 110xxxxx */
    if ((lead & 0xF0) == 0xE0) return 2;            /* 1110xxxx */
    if ((lead & 0xF8) == 0xF0) return 3;            /* 11110xxx */
    return 0;                                       /* 10xxxxxx or 11111xxx */
}

size_t utf8_decode(const unsigned char* s, size_t avail, uint32_t* cp)
{
    if (cp) *cp = 0;
    if (!s || avail == 0) return 0;

    unsigned char b = s[0];
    int n = utf8_trail_count(b);

    if (n == 0) {
        if (cp) *cp = b;
        return 1;
    }

    /* Not enough bytes left in the buffer: decode the lead alone rather
     * than reading past the end. */
    if ((size_t)n + 1u > avail) {
        if (cp) *cp = b;
        return 1;
    }

    uint32_t code = (uint32_t)(b & (0x7Fu >> n));
    for (int i = 1; i <= n; i++) {
        unsigned char cb = s[i];
        if ((cb & 0xC0) != 0x80) {                  /* invalid continuation */
            if (cp) *cp = b;
            return 1;
        }
        code = (code << 6) | (cb & 0x3Fu);
    }

    /* Reject overlong / out-of-range forms the same way as a bad lead. */
    if (code > 0x10FFFFu) {
        if (cp) *cp = b;
        return 1;
    }

    if (cp) *cp = code;
    return (size_t)n + 1u;
}

uint32_t codepoint_width(uint32_t cp)
{
    if (cp < 0x20 || cp == 0x7F) return 0;          /* control (handled elsewhere) */

    /* Zero-width / combining marks. */
    if (cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0x2060 ||
        cp == 0xFEFF) {
        return 0;
    }
    if ((cp >= 0x0300 && cp <= 0x036F) ||           /* combining diacritics */
        (cp >= 0x1AB0 && cp <= 0x1AFF) ||
        (cp >= 0x1DC0 && cp <= 0x1DFF) ||
        (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0xFE00 && cp <= 0xFE0F)) {           /* variation selectors */
        return 0;
    }

    /* East-Asian Wide / Fullwidth / emoji: occupy two columns. */
    if (cp >= 0x1100 && cp <= 0x115F) return 2;     /* Hangul Jamo */
    if (cp >= 0x2E80 && cp <= 0x303E) return 2;     /* CJK radicals, Kangxi */
    if (cp >= 0x3041 && cp <= 0x33FF) return 2;     /* Hiragana..CJK symbols */
    if (cp >= 0x3400 && cp <= 0x4DBF) return 2;     /* CJK Ext A */
    if (cp >= 0x4E00 && cp <= 0x9FFF) return 2;     /* CJK Unified */
    if (cp >= 0xA000 && cp <= 0xA4CF) return 2;     /* Yi */
    if (cp >= 0xAC00 && cp <= 0xD7A3) return 2;     /* Hangul Syllables */
    if (cp >= 0xF900 && cp <= 0xFAFF) return 2;     /* CJK Compatibility */
    if (cp >= 0xFE30 && cp <= 0xFE4F) return 2;     /* CJK Compat forms */
    if (cp >= 0xFF00 && cp <= 0xFF60) return 2;     /* Fullwidth forms */
    if (cp >= 0xFFE0 && cp <= 0xFFE6) return 2;     /* Fullwidth symbols */
    if (cp >= 0x1F300 && cp <= 0x1FAFF) return 2;   /* emoji & symbols */
    if (cp >= 0x1F000 && cp <= 0x1FFFF) return 2;   /* supplemental symbols */
    if (cp >= 0x20000 && cp <= 0x3FFFD) return 2;   /* CJK Ext B+ */

    return 1;
}

uint32_t char_width(const unsigned char* s, size_t avail, uint32_t col,
                    uint32_t tab_size, size_t* consumed)
{
    if (consumed) *consumed = 0;
    if (!s || avail == 0) return 0;

    unsigned char b = s[0];

    if (b == '\t') {
        uint32_t ts = (tab_size > 0) ? tab_size : 1;
        if (consumed) *consumed = 1;
        return ts - (col % ts);
    }
    if (b < 0x20 || b == 127) {
        if (consumed) *consumed = 1;
        return 2;                                   /* caret pair */
    }

    uint32_t cp;
    size_t n = utf8_decode(s, avail, &cp);
    if (consumed) *consumed = n;
    return codepoint_width(cp);
}
