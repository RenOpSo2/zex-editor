#ifndef NODES_H
#define NODES_H

#include "global.h"

void pgb_init(struct paged_gap_buffer* pgb, Arena* arena);
void pgb_insert(struct paged_gap_buffer* pgb, char ch, Arena* arena);
void pgb_delete(struct paged_gap_buffer* pgb);
void pgb_clear(struct paged_gap_buffer* pgb);
void pgb_insert_str(struct paged_gap_buffer* pgb, const char* src, Arena* arena);
void pgb_replace_str(struct paged_gap_buffer* pgb, const char* src, Arena* arena);
void pgb_to_str(char* dst, size_t dst_size, struct paged_gap_buffer* pgb);
void pgb_move_left(struct paged_gap_buffer* pgb);
void pgb_move_right(struct paged_gap_buffer* pgb);
void pgb_move_up(struct paged_gap_buffer* pgb);
void pgb_move_down(struct paged_gap_buffer* pgb);

// Selection & clipboard helpers
uint32_t pgb_cursor_pos(struct paged_gap_buffer* pgb);
void     pgb_move_to_pos(struct paged_gap_buffer* pgb, uint32_t pos);
void     pgb_copy_range(struct paged_gap_buffer* dst, struct paged_gap_buffer* src,
                        uint32_t from, uint32_t to, Arena* arena);
void     pgb_delete_range(struct paged_gap_buffer* pgb, uint32_t from, uint32_t to);

// Undo/redo functions
void undo_perform(struct global* global);
void redo_perform(struct global* global);
void undo_save_insert(struct global* global, char ch, uint32_t pos);
void undo_save_delete(struct global* global, char ch, uint32_t pos);

// Search functions
void search_init(struct global* global);
void search_find(struct global* global, const char* query);
void search_next(struct global* global);
void search_prev(struct global* global);

#endif
