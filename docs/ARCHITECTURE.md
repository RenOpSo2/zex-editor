# Zex - Architecture Overview

This is the architecture documentation for the Zex terminal text editor project.
The architecture is designed for efficiency with no external library dependencies.

## Project Structure
```

src/
├── global.h    # Shared types, enums, structs
├── main.c      # Entry point
├── editor.c/h  # Core orchestrator (init/update/deinit)
├── nodes.c/h   # Paged Gap Buffer (text storage)
├── input.c/h   # Keyboard and mouse input handling
├── draw.c/h    # Terminal rendering
├── file.cpp/h  # File read/write operations
├── term.cpp/h  # Terminal raw mode I/O
├── syntax.c/h  # Syntax highlighting
├── config.c/h  # Configuration system
├── cmd.c/h     # Command handling
├── render_buffer.cpp/h  # Double-buffered rendering
├── dispwidth.cpp/h      # UTF-8 display width calculations
└── undo_rendo.cpp       # Undo/redo implementation (C++)

libmemory/      # Arena Allocator (custom memory management)
├── arena.c
├── arena.h
└── examples.c
```

## Key Components

### Core Editor Loop
```
main.c
  ↓
editor_init() → Setup terminal, load file, init structures
  ↓
editor_update() → Main loop
  ↓
  - input.c → Handle keyboard/mouse input
  - nodes.c → Update text buffer (Paged Gap Buffer)
  - draw.c → Render to render buffer
  - render_buffer.cpp → Flush to terminal
  ↓
editor_deinit() → Cleanup, save file
```

### Data Structures

**Paged Gap Buffer** (`src/nodes.c`)
- Linked list of fixed-size pages (4KB each)
- Each page has a gap for O(1) insert/delete at cursor
- Efficient for large files due to paged design
- Supports undo/redo, search, selection, clipboard

**Arena Allocator** (`libmemory/arena.c`)
- Bump allocator for O(1) allocations
- All memory freed at once with arena_reset()
- Used for text buffer nodes and temporary allocations
- 16MB arena capacity per editor session

**Render Buffer** (`src/render_buffer.cpp`)
- Double-buffered rendering to prevent flicker
- Grows dynamically up to 64MB max
- ANSI escape sequence buffering
- Efficient write batching to terminal

### Memory Management

The editor uses a hybrid approach:
- **Arena Allocator** for text storage and editor state (fast, bulk cleanup)
- **Standard malloc/free** for render buffer (dynamic growth)

Example arena usage:
```c
// Initialize arena
static char arena_memory[arena_capacity];
Arena arena = arena_init(arena_memory, arena_capacity);

// Allocate page
struct page* p = arena_new(&arena, struct page);

// Reset arena (free all allocations at once)
arena_reset(&arena);
```

### Terminal Interface

**Raw Mode** (`src/term.cpp`)
- Disables line buffering and echo
- Enables single-character input
- Handles X10 mouse protocol for click events
- ANSI escape sequences for cursor movement and colors

**Display Width** (`src/dispwidth.cpp`)
- UTF-8 decoding with error handling
- East Asian character width support (CJK, emoji)
- Tab expansion logic
- Control character caret notation

### Configuration System

**Config Loading** (`src/config.c`)
- Loads from `~/.zexrc`, `./zex.json`, and CLI args
- JSON parsing with validation
- Runtime file watching for hot-reload
- Schema validation for type safety

Example config structure:
```json
{
  "tabsize": 4,
  "mouse": true,
  "show_line_numbers": true,
  "auto_indent": true
}
```

## Performance Characteristics

- **Insert/Delete**: O(1) at cursor position (gap buffer)
- **Navigation**: O(1) within page, O(n) across pages
- **File Load**: O(n) where n is file size
- **Rendering**: O(screen_size) per frame
- **Memory**: O(file_size) with 16MB arena limit

## Language Mix

The project uses both C and C++:
- **C**: Core editor logic, input handling, syntax highlighting
- **C++**: File I/O, terminal interface, render buffer, UTF-8 handling
- **extern "C"**: Used in C++ files for C compatibility




