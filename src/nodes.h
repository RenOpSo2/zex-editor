#ifndef NODES_H
#define NODES_H

#include "global.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void pgb_init(struct paged_gap_buffer* pgb, Arena* arena);
void pgb_insert(struct paged_gap_buffer* pgb, char ch, Arena* arena);
void pgb_delete(struct paged_gap_buffer* pgb);
void pgb_clear(struct paged_gap_buffer* pgb);
void pgb_insert_str(struct paged_gap_buffer* pgb, const char* src, Arena* arena);
void pgb_replace_str(struct paged_gap_buffer* pgb, const char* src, Arena* arena);
void pgb_to_str(char* dst, size_t dst_size, const struct paged_gap_buffer* pgb);

/**
 * pgb_reader - read-only cursor over the logical bytes of a buffer.
 *
 * Walks page by page and yields each stored byte exactly once, skipping
 * the gaps, so consumers can stream arbitrarily large documents without
 * flattening them into an intermediate buffer. NUL bytes are ordinary
 * data: iteration is length-driven, never terminator-driven.
 */
struct pgb_reader {
    const struct page* page;
    uint32_t idx;  /* offset within the current segment of page */
    uint8_t phase; /* 0 = before gap, 1 = after gap, 2 = exhausted */
};

void pgb_reader_init(struct pgb_reader* it, const struct paged_gap_buffer* pgb);

/** Returns the next logical byte (0..255), or -1 when exhausted. */
int pgb_reader_next(struct pgb_reader* it);

void pgb_move_left(struct paged_gap_buffer* pgb);
void pgb_move_right(struct paged_gap_buffer* pgb);
void pgb_move_up(struct paged_gap_buffer* pgb);
void pgb_move_down(struct paged_gap_buffer* pgb);

// Selection & clipboard helpers
uint32_t pgb_cursor_pos(const struct paged_gap_buffer* pgb);
void     pgb_move_to_pos(struct paged_gap_buffer* pgb, uint32_t pos);
void     pgb_copy_range(struct paged_gap_buffer* dst, const struct paged_gap_buffer* src,
                        uint32_t from, uint32_t to, Arena* arena);
void     pgb_delete_range(struct paged_gap_buffer* pgb, uint32_t from, uint32_t to);

// Undo/redo functions
void undo_perform(struct global* global);
void redo_perform(struct global* global);
void undo_save_insert(struct global* global, char ch, uint32_t pos);
void undo_save_delete(struct global* global, char ch, uint32_t pos);
void undo_save_replace(struct global* global, const char* new_text, uint32_t new_len, 
                       const char* old_text, uint32_t old_len, uint32_t pos);
void undo_save_batch_insert(struct global* global, const char* text, uint32_t len, uint32_t pos);
void undo_save_batch_delete(struct global* global, const char* text, uint32_t len, uint32_t pos);
void undo_clear_history(struct global* global);
bool undo_can_undo(struct global* global);
bool undo_can_redo(struct global* global);

// Search functions
void search_init(struct global* global);
void search_find(struct global* global, const char* query);
void search_next(struct global* global);
void search_prev(struct global* global);

#ifdef __cplusplus
}
#endif

#endif
