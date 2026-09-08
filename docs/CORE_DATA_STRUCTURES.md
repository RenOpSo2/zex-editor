# Core Data Structures

This document describes the fundamental data structures used in Zex, focusing on the Paged Gap Buffer which is the heart of the text editor.

## Paged Gap Buffer

The Paged Gap Buffer is the core data structure for text storage. It combines the efficiency of gap buffers with the scalability of paged memory management.

### Concept

A traditional gap buffer maintains a single contiguous gap where edits occur. Zex extends this concept by using multiple pages (4KB each) linked together, each with its own gap.

**Advantages:**
- O(1) insert/delete at cursor position
- Handles large files efficiently (paged design)
- Memory-efficient (no contiguous memory requirement)
- Good cache locality within pages

### Structure

```c
// Individual page (4KB capacity)
struct page {
    char data[PAGE_CAPACITY];      // 4096 bytes storage
    uint32_t gap_start;            // Start of gap (cursor position)
    uint32_t gap_end;              // End of gap (unused space)
    struct page* next;              // Next page in linked list
    struct page* prev;              // Previous page in linked list
};

// Overall buffer
struct paged_gap_buffer {
    struct page* head;              // First page
    struct page* tail;              // Last page
    struct page* active_page;       // Page containing cursor
};
```

### Page Layout

```
[page.data] example (gap_start=10, gap_end=20):
[Content 0-9][GAP 10-19][Content 20-4095]
     ↑              ↑
   gap_start     gap_end
```

The gap represents unused space where new text can be inserted without moving existing data.

### Basic Operations

#### Initialization

```c
void pgb_init(struct paged_gap_buffer* pgb, Arena* arena)
{
    pgb->head = page_new(arena);      // Create first page
    pgb->tail = pgb->head;
    pgb->active_page = pgb->head;
    // New page starts with full gap (gap_start=0, gap_end=4096)
}
```

#### Insert Character

```c
void pgb_insert(struct paged_gap_buffer* pgb, char ch, Arena* arena)
{
    struct page* curr = pgb->active_page;
    
    // If gap is full, split the page
    if (curr->gap_start == curr->gap_end) {
        page_split(pgb, arena);
        curr = pgb->active_page;
    }
    
    // Insert at gap_start and move gap
    curr->data[curr->gap_start] = ch;
    curr->gap_start++;
}
```

#### Delete Character

```c
void pgb_delete(struct paged_gap_buffer* pgb)
{
    struct page* curr = pgb->active_page;
    
    // If there's content before gap, delete it
    if (curr->gap_start > 0) {
        curr->gap_start--;  // Move gap backward (consumes character)
    }
    // If at page start, move to previous page
    else if (curr->prev) {
        pgb->active_page = curr->prev;
        pgb_delete(pgb);   // Delete from previous page
    }
}
```

#### Page Split

When a page's gap is exhausted, it splits:

```c
static void page_split(struct paged_gap_buffer* pgb, Arena* arena)
{
    struct page* curr = pgb->active_page;
    struct page* new_page = page_new(arena);
    
    // Calculate content after gap
    uint32_t right_len = PAGE_CAPACITY - curr->gap_end;
    
    // Move right half to new page
    memcpy(new_page->data + new_page->gap_end, 
           curr->data + curr->gap_end, right_len);
    
    // Current page now has no content after gap
    curr->gap_end = PAGE_CAPACITY;
    
    // Link new page after current
    new_page->prev = curr;
    new_page->next = curr->next;
    if (curr->next) curr->next->prev = new_page;
    else pgb->tail = new_page;
    curr->next = new_page;
}
```

### Cursor Movement

The cursor position is represented by the gap boundaries. Moving the cursor shifts the gap:

```c
void pgb_move_right(struct paged_gap_buffer* pgb)
{
    struct page* curr = pgb->active_page;
    
    // If there's content after gap, move gap right
    if (curr->gap_end < PAGE_CAPACITY) {
        curr->data[curr->gap_start] = curr->data[curr->gap_end];
        curr->gap_start++;
        curr->gap_end++;
    }
    // If at page end, move to next page
    else if (curr->next) {
        pgb->active_page = curr->next;
        pgb->active_page->gap_start = 0;
        pgb->active_page->gap_end = PAGE_CAPACITY;
    }
}
```

### Paged Gap Buffer Reader

For operations that need to read the entire buffer (like file save), a reader iterator is provided:

```c
struct pgb_reader {
    const struct page* page;
    uint32_t idx;      // Offset within current segment
    uint8_t phase;     // 0=before gap, 1=after gap, 2=exhausted
};

// Initialize reader
void pgb_reader_init(struct pgb_reader* it, const struct paged_gap_buffer* pgb);

// Get next byte (returns -1 when exhausted)
int pgb_reader_next(struct pgb_reader* it);
```

This allows streaming arbitrarily large documents without flattening them into a single buffer.

### Performance Characteristics

| Operation | Time Complexity | Notes |
|-----------|----------------|-------|
| Insert at cursor | O(1) | Unless page split needed |
| Delete at cursor | O(1) | Simple gap adjustment |
| Move cursor | O(1) | Within page |
| Move across pages | O(n) | n = number of pages |
| File save | O(total_bytes) | Linear scan with reader |

### Memory Usage

- Each page: 4KB + struct overhead
- Arena allocator: 16MB capacity per session
- Typical usage: ~1.1x file size (gap overhead)

### Usage Example

```c
// Initialize
Arena arena = arena_init(memory_buffer, arena_capacity);
struct paged_gap_buffer buffer;
pgb_init(&buffer, &arena);

// Insert text
pgb_insert(&buffer, 'H', &arena);
pgb_insert(&buffer, 'e', &arena);
pgb_insert(&buffer, 'l', &arena);
pgb_insert(&buffer, 'l', &arena);
pgb_insert(&buffer, 'o', &arena);

// Move cursor
pgb_move_left(&buffer);
pgb_move_left(&buffer);

// Delete
pgb_delete(&buffer);  // Deletes 'l'

// Export to string
char output[buf_capacity];
pgb_to_str(output, sizeof(output), &buffer);
// Result: "Helo"
```

## Arena Allocator

The Arena Allocator is a bump allocator used for fast, bulk memory management.

### Concept

Memory is allocated sequentially from a pre-allocated buffer. All allocations are freed at once by resetting the offset.

### Structure

```c
typedef struct Arena {
    char* buf;        // Underlying memory buffer
    size_t offset;    // Current allocation position
    size_t capacity;  // Total buffer size
} Arena;
```

### Basic Usage

```c
// Initialize
static char arena_memory[arena_capacity];
Arena arena = arena_init(arena_memory, arena_capacity);

// Allocate single object
struct page* p = arena_new(&arena, struct page);

// Allocate array
int* numbers = arena_new_array(&arena, int, 100);

// Allocate and zero
struct page* p = arena_cnew(&arena, struct page);

// Duplicate string
char* copy = arena_strdup(&arena, "hello");

// Free all allocations at once
arena_reset(&arena);
```

### Performance

- Allocation: O(1) - just bump offset
- Free individual: Not supported (by design)
- Free all: O(1) - reset offset to 0

### Advantages

- Extremely fast allocations
- No fragmentation
- Perfect for text editor sessions (clear all on file close)
- Simplifies memory management (no individual frees)

### When to Use

**Use Arena when:**
- Allocations have similar lifetime (e.g., one editing session)
- You need bulk cleanup (e.g., close file, new file)
- Performance is critical

**Use malloc/free when:**
- Objects have independent lifetimes
- You need individual deallocation
- Memory needs to grow dynamically

## Render Buffer

The Render Buffer is a dynamic buffer for double-buffered terminal output.

### Purpose

Prevents screen flickering by:
1. Building complete frame in memory
2. Flushing entire frame to terminal at once
3. Avoiding partial screen updates

### Structure

```c
typedef struct RenderBuffer {
    char* data;        // Buffer contents
    size_t len;        // Current length
    size_t cap;        // Capacity
    int overflowed;   // Flag if capacity exceeded
} RenderBuffer;
```

### Usage

```c
RenderBuffer rb;
rb_init(&rb);

// Append data
rb_append(&rb, "Hello", 5);
rb_append_char(&rb, ' ');
rb_append_str(&rb, "World");

// Flush to terminal
rb_flush(&rb);

// Clear for next frame
rb_clear(&rb);
```

### Growth Strategy

- Initial capacity: 4KB
- Grows by doubling up to 64MB max
- Handles partial writes gracefully
- Tracks overflow for error handling

## Global State Structure

The main editor state is centralized in a single structure:

```c
struct global {
    struct term term;                    // Terminal info
    struct paged_gap_buffer text;        // Main text buffer
    struct paged_gap_buffer clipboard;   // Clipboard buffer
    struct paged_gap_buffer msg;          // Message buffer
    
    char filepath[MAX_FILEPATH_LEN];     // Current file path
    
    // Selection state
    bool has_selection;
    uint32_t sel_anchor;
    
    // Undo/redo stacks
    struct action undo_stack[UNDO_STACK_SIZE];
    struct action redo_stack[UNDO_STACK_SIZE];
    uint32_t undo_count;
    uint32_t redo_count;
    
    // Search state
    bool search_active;
    char search_query[MAX_SEARCH_QUERY_LEN];
    uint32_t search_pos;
    uint32_t search_match_count;
    
    Arena arena;                          // Memory arena
};
```

This single-structure approach simplifies state management and ensures all editor state is available to all components.