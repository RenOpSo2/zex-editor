# Noxe - Architecture Overview

This is the architecture documentation for the node-editor project, which you can view here.
The architecture is clean and very easy to understand.

## Project Structure
```

src/
├── global.h    # Shared types, enums, structs
├── term.c/h    # Terminal raw mode I/O
├── nodes.c/h   # Text buffer (doubly-linked list)
├── file.c/h    # File read/write operations
├── cmd.c/h     # Command parser & executor
├── input.c/h   # Modal input handler (state machine)
├── draw.c/h    # Terminal rendering
├── editor.c/h  # Core orchestrator (init/update/deinit)
└── main.c      # Entry point
libmemory      # Arena Allocator (must use this, avoid malloc)
├── arena.c    
├── arena.h    # Header and docs libmemory
└── examples.c 
```




