# Noxe-Editor — Critical Bug Report

Scope: critical-severity bugs only (crashes / data corruption). Non-critical issues
(performance, cosmetic, 64KB soft limits on search/render) were observed but are
out of scope for this pass.

---

## BUG-1 (Critical — crash): Editor segfaults when its memory arena is exhausted

**Where:** `src/nodes.c` — `page_new()`, called from `pgb_init()` and `page_split()`.

**What happens:**
All text pages come from a single fixed 16 MB arena (`arena_capacity` in
`src/global.h`). `page_new()` calls `arena_cnew()` to allocate a `struct page`
but never checks the result:

```c
static struct page* page_new(Arena* arena) {
    struct page* p = arena_cnew(arena, struct page);
    p->gap_start = 0;   // <- NULL deref if the arena is full
    ...
}
```

`arena_calloc`/`arena_alloc` return `NULL` once the arena's 16 MB is used up.
Because pages are never freed (append-only paged gap buffer) and each page is
~4 KB, the arena fills after roughly 4,000 pages — reachable by opening a large
file, pasting a lot of text, or just editing for a long session. The very next
`page_new()` call dereferences `NULL`, and the editor crashes with a segfault,
losing any unsaved work. This directly contradicts the advertised "Large file
support" feature.

**Reproduction (conceptually):** open/create a file large enough to consume the
16 MB arena (e.g. ~16 MB of content, or many pages from heavy editing), keep
typing/pasting — the process segfaults.

**Fix:** propagate the allocation failure instead of dereferencing it, and make
every caller degrade gracefully (drop the edit) instead of crashing or writing
out of bounds.

---

## BUG-2 (Critical — silent data corruption): Undo after Backspace can restore the wrong character in the wrong place

**Where:** `src/input.c`, backspace handling in `input_update()` (around the
`ch == '\b' || ch == 127` branch).

**What happens:**
```c
char buffer[buf_capacity];
pgb_to_str(buffer, sizeof(buffer), &global->text);
uint32_t len = strlen(buffer);
char deleted_char = (len > 0) ? buffer[len - 1] : ch;   // always the LAST char of the doc

uint32_t pos = pgb_cursor_pos(&global->text);           // cursor pos BEFORE deletion
pgb_delete(&global->text);
undo_save_delete(global, deleted_char, pos);
```

Two independent mistakes are combined here:

1. `deleted_char` is read from the **end of the whole document**, not from the
   character actually being removed. Backspacing anywhere except the very end
   of the file records the wrong character.
2. `pos` is the cursor position **before** the delete. `undo_perform()` later
   calls `pgb_move_to_pos(act->pos)` and inserts there — but after a delete the
   correct restore position is `pos - 1`, not `pos`. Using the pre-delete
   position places the restored character one slot too far to the right.

Both bugs happen to cancel out only in the special case where the user is
backspacing at the very end of the document (the most common manual test),
which is why this was not caught before. Anywhere else — e.g. backspacing in
the middle of a line after using arrow keys — pressing Ctrl+U silently inserts
the wrong character at the wrong offset, corrupting the file content without
any error message.

**Fix:** capture the cursor position **before** deleting, read the character
at `pos - 1` from the buffer (the one actually being deleted), and record
`pos - 1` (not `pos`) as the undo position. Also skip recording anything when
`pos == 0` (nothing to delete).

---

## Fix status
Both bugs are fixed in this pass — see the corresponding commit / diff:
- BUG-1: `src/nodes.c` (`page_new`, `page_split`, `pgb_insert`)
- BUG-2: `src/input.c` (backspace handler)
