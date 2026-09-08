# Developer Guide

This guide helps developers get started with contributing to Zex, covering build, test, debug, and development workflow.

## Prerequisites

- GCC (for C files)
- G++ (for C++ files)
- Make (build system)
- clang-format (code formatting)
- cppcheck (static analysis, optional)

## Building the Project

### Quick Start

```bash
# Build the project
make

# Or build and run
make run

# Build specific target
make all
```

### Build Outputs

- **Binary**: `bin/zex` - The main editor executable
- **Object files**: `build/` - Compiled object files
- **Dependencies**: `build/*.d` - Auto-generated dependency files

### Build Targets

```bash
make              # Build everything (default)
make run          # Build and run
make clean        # Remove build artifacts
make format       # Format source code
make format-check # Check formatting without changes
make check        # Run static analysis with cppcheck
make release      # Create release (VERSION=x.y.z required)
make help         # Show all available targets
```

### Compiler Flags

The project uses strict compiler flags for safety:

**C files:**
```makefile
CFLAGS = -Wall -Wextra -Wpedantic -O2 -std=gnu99 -I.
```

**C++ files:**
```makefile
CXXFLAGS = -Wall -Wextra -Wpedantic -O2 -std=c++17 -I.
```

## Testing

### Running All Tests

```bash
make test
```

### Running Individual Tests

```bash
make test-stress       # Stress test
make test-edge         # Edge case test
make test-cursor        # Cursor accuracy test
make test-cpp-bridge   # C++ bridge test
make test-enhanced-undo # Enhanced undo test
make test-large-undo   # Large undo test
make test-file-io      # File I/O test
make test-search       # Search test
make test-selection    # Selection test
make test-cmd          # Command test
make test-auto-indent  # Auto-indent test
```

### Test Files

Test files are located in `tests/`:
- `test_undo.c` - Basic undo/redo functionality
- `test_enhanced_undo.c` - Enhanced undo features
- `test_large_undo.c` - Large file undo performance
- `test_0.2.0.c` - Version 0.2.0 regression tests
- `test_config.c` - Configuration system tests
- `stress_test.c` - Performance stress tests
- `edge_case_test.c` - Edge case handling
- `test_cursor.c` - Cursor movement accuracy
- `test_file_io.c` - File read/write operations
- `test_search.c` - Search functionality
- `test_selection.c` - Text selection
- `test_cmd.c` - Command handling
- `test_auto_indent.c` - Auto-indent behavior

### Writing Tests

Test structure follows this pattern:

```c
#include "nodes.h"
#include "global.h"
#include <stdio.h>
#include <assert.h>

int main(void)
{
    // Setup
    Arena arena;
    // ... initialize arena
    
    struct paged_gap_buffer buffer;
    pgb_init(&buffer, &arena);
    
    // Test case
    pgb_insert(&buffer, 'H', &arena);
    pgb_insert(&buffer, 'i', &arena);
    
    // Verify
    char output[buf_capacity];
    pgb_to_str(output, sizeof(output), &buffer);
    assert(strcmp(output, "Hi") == 0);
    
    printf("Test passed!\n");
    return 0;
}
```

## Debugging

### Using GDB

```bash
# Build with debug symbols
make clean
make CFLAGS="-Wall -Wextra -Wpedantic -g -O0 -std=gnu99 -I."

# Run with GDB
gdb bin/zex

# Common GDB commands
(gdb) break input_update        # Set breakpoint
(gdb) run                        # Start program
(gdb) continue                   # Continue execution
(gdb) print global.text.head     # Print variable
(gdb) step                       # Step through code
(gdb) backtrace                  # Show call stack
```

### Using printf Debugging

Add temporary print statements:

```c
void pgb_insert(struct paged_gap_buffer* pgb, char ch, Arena* arena)
{
    printf("Inserting '%c' at page %p\n", ch, pgb->active_page);
    // ... rest of function
}
```

### Common Debugging Scenarios

**Memory Issues:**
```bash
# Use Valgrind to check for memory leaks
valgrind --leak-check=full ./bin/zex testfile.c
```

**Static Analysis:**
```bash
# Run cppcheck for potential issues
make check
```

**Formatting Issues:**
```bash
# Check if code is properly formatted
make format-check

# Auto-format code
make format
```

## Code Style

### Formatting

The project uses clang-format for consistent code style:

```bash
# Format all source files
make format

# Check formatting without changes
make format-check
```

### Style Guidelines

- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style
- **Line length**: Prefer under 80 characters
- **Naming**: snake_case for functions/variables, UPPER_CASE for constants

Example:
```c
void pgb_insert(struct paged_gap_buffer* pgb, char ch, Arena* arena)
{
    if (!pgb || !arena) return;
    
    struct page* curr = pgb->active_page;
    if (curr->gap_start == curr->gap_end) {
        page_split(pgb, arena);
    }
    
    curr->data[curr->gap_start] = ch;
    curr->gap_start++;
}
```

### Comments

- Use Doxygen-style comments for public functions
- Keep comments concise and relevant
- Document non-obvious logic

Example:
```c
/**
 * pgb_insert - Insert character at cursor position
 * @pgb: Pointer to paged gap buffer
 * @ch: Character to insert
 * @arena: Memory arena for allocations
 * 
 * Inserts character at current cursor position (gap_start).
 * Splits page if gap is exhausted.
 */
void pgb_insert(struct paged_gap_buffer* pgb, char ch, Arena* arena);
```

## Development Workflow

### Making Changes

1. **Create feature branch** (optional but recommended)
   ```bash
   git checkout -b feature/my-feature
   ```

2. **Make changes** to source files

3. **Format code**
   ```bash
   make format
   ```

4. **Build**
   ```bash
   make clean && make
   ```

5. **Test**
   ```bash
   make test
   ```

6. **Run specific tests** if relevant
   ```bash
   make test-undo
   ```

7. **Manual testing**
   ```bash
   ./bin/zex test_file.c
   ```

### Common Tasks

**Adding a new keybinding:**
1. Edit `src/input.c` - Add case in `input_update()`
2. Update `README.md` - Document the new binding
3. Test with manual run

**Adding a new config option:**
1. Edit `src/config.c` - Add to `config_init()`
2. Add getter/setter if needed
3. Update documentation
4. Test with `make test-config`

**Adding syntax highlighting:**
1. Edit `src/syntax.c` - Add keywords/types lists
2. Update `syntax_get_language()` for file extension
3. Test with sample file

## Project Structure Overview

```
zex/
├── src/                    # Source code
│   ├── main.c             # Entry point
│   ├── editor.c/h         # Core editor logic
│   ├── nodes.c/h          # Paged Gap Buffer
│   ├── input.c/h          # Input handling
│   ├── draw.c/h           # Rendering
│   ├── file.cpp/h         # File I/O
│   ├── term.cpp/h         # Terminal interface
│   ├── syntax.c/h         # Syntax highlighting
│   ├── config.c/h         # Configuration
│   ├── render_buffer.cpp/h# Render buffer
│   ├── dispwidth.cpp/h    # UTF-8 width
│   └── global.h           # Shared types
├── libmemory/             # Arena allocator
│   ├── arena.c
│   └── arena.h
├── tests/                 # Test files
├── docs/                  # Documentation
├── Makefile               # Build system
└── README.md              # User documentation
```

## Memory Management Guidelines

### Use Arena Allocator for Text Storage

```c
// GOOD - Use arena for text buffer
Arena arena = arena_init(memory, capacity);
struct page* p = arena_new(&arena, struct page);

// AVOID - Don't use malloc for editor state
struct page* p = malloc(sizeof(struct page));
```

### Use Standard malloc for Dynamic Growth

```c
// GOOD - Use malloc for render buffer (dynamic growth)
rb->data = malloc(new_capacity);

// AVOID - Don't use arena for unlimited growth
rb->data = arena_alloc(&arena, new_capacity, 1);
```

### Reset Arena on File Close

```c
// When closing a file, reset the arena
arena_reset(&global->arena);
pgb_init(&global->text, &global->arena);
```

## Common Pitfalls

### 1. Forgetting to Check Arena Allocation

```c
// BAD - Doesn't check for NULL
struct page* p = arena_new(&arena, struct page);
p->data[0] = 'x';  // Crash if allocation failed

// GOOD - Check for NULL
struct page* p = arena_new(&arena, struct page);
if (!p) {
    // Handle allocation failure
    return;
}
p->data[0] = 'x';
```

### 2. Mixing C and C++ String Handling

```c
// BAD - Using C++ string functions in C file
std::string s = "hello";  // Won't compile in .c file

// GOOD - Use C string functions
char* s = arena_strdup(&arena, "hello");
```

### 3. Not Updating Undo Stack

```c
// BAD - Modifying text without saving undo
pgb_insert(&buffer, 'x', &arena);

// GOOD - Save undo state
undo_save_insert(&global, 'x', position);
pgb_insert(&buffer, 'x', &arena);
```

### 4. Ignoring Return Values

```c
// BAD - Ignoring error return
file_write(path, &buffer);

// GOOD - Check return value
enum result res = file_write(path, &buffer);
if (res != ok) {
    // Handle error
}
```

## Performance Considerations

### Optimization Tips

1. **Minimize arena resets** - Reset only when necessary (file close)
2. **Batch operations** - Use `pgb_insert_str()` instead of multiple `pgb_insert()`
3. **Limit rendering** - Only render visible lines
4. **Cache calculations** - Avoid repeated expensive operations

### Performance Testing

```bash
# Run stress test
make test-stress

# Manual performance test
time ./bin/zex large_file.c
```

## Getting Help

- **Architecture**: See `docs/ARCHITECTURE.md`
- **Data Structures**: See `docs/CORE_DATA_STRUCTURES.md`
- **Project Structure**: See `docs/STRUKTUR_PROJECT.md`
- **Function Reference**: See `docs/function.md`

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Ensure tests pass: `make test`
5. Format code: `make format`
6. Submit a pull request

Ensure your changes:
- Follow the code style guidelines
- Include tests for new functionality
- Update relevant documentation
- Don't break existing tests