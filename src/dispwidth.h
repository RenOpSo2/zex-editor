#ifndef DISPWIDTH_H
#define DISPWIDTH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Display-width abstraction.
 *
 * The editor renders text in display columns, but documents are stored as
 * UTF-8 bytes. A single on-screen cell can be:
 *   - one ASCII byte            (1 byte,  1 column)
 *   - part of a UTF-8 sequence  (2..4 bytes, 1 column for most, 2 for wide)
 *   - a tab                     (1 byte, 1..tab_size columns)
 *   - a control character       (1 byte, 2 columns as a caret pair)
 *
 * Every place that turns bytes into display columns must agree, otherwise the
 * cursor, the rendered text and the mouse hit-test drift apart. These helpers
 * are the single source of truth for that conversion so the renderer and the
 * cursor math can never disagree.
 */

/* Number of continuation bytes implied by a UTF-8 lead byte.
 * Returns 0 for ASCII or for a byte that is not a valid lead. */
int utf8_trail_count(unsigned char lead);

/* Decode the UTF-8 character beginning at s (avail bytes available).
 * On success stores the codepoint in *cp and returns the byte count (1..4).
 * On a truncated or malformed sequence it falls back to treating the lead
 * byte as one raw byte (returns 1), so callers never desynchronise. */
size_t utf8_decode(const unsigned char* s, size_t avail, uint32_t* cp);

/* Display columns occupied by a decoded codepoint. Printable ASCII and the
 * majority of Unicode count as 1; East-Asian wide / fullwidth / emoji count
 * as 2; zero-width (combining) characters count as 0. Tabs and control
 * characters are intentionally NOT handled here. */
uint32_t codepoint_width(uint32_t cp);

/* Display columns occupied by the character beginning at s (avail bytes).
 * Tabs are expanded against the current column `col` using `tab_size`;
 * control characters (< 0x20 and DEL) count as a 2-column caret pair.
 * *consumed receives the number of bytes making up the character so the
 * caller can advance its stream by exactly that amount. */
uint32_t char_width(const unsigned char* s, size_t avail, uint32_t col,
                    uint32_t tab_size, size_t* consumed);

#ifdef __cplusplus
}
#endif

#endif