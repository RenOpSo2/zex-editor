# Zex

## Overview
Zex is a terminal-based text editor written in C, similar in spirit to vim/nano.
It was imported from GitHub. It is a CLI/TUI application — there is no web UI or browser
preview.

Features: auto-indent, undo/redo, C syntax highlighting, a paged gap buffer for large
files, clipboard (copy/cut/paste), selection, incremental search, line numbers, and an
arena allocator for memory management.

## Tech Stack
- Language: C (gnu99)
- Build system: `make` (GNU Makefile)
- No external dependencies beyond a C toolchain (gcc)

## Running the project
The `Run Zex Editor` workflow builds the project (`make`) and launches the compiled
binary (`./bin/zex`) in the console pane, so you can interact with it directly there.

To run it manually from the Shell:
```bash
make            # builds to bin/zex
./bin/zex [filename]   # open a file, or omit for an empty buffer
```

Key bindings: Ctrl+S save, Ctrl+Q quit, Ctrl+F search (Ctrl+N/P to navigate matches),
Ctrl+U undo, Ctrl+Y redo, Ctrl+C/X/V copy/cut/paste, Ctrl+A select all, Shift+Arrows to
select, Ctrl+R refresh screen. Full details in `README.md`.

## Project structure
- `src/` — editor source files (draw, editor, file, input, main, nodes, render_buffer,
  syntax, term)
- `libmemory/` — arena allocator used for memory management
- `test_undo.c`, `test_0.2.0.c`, `test_main.c` — test programs, buildable via the
  Makefile's test targets
- `docs/`, `Changelog`, `CHANGELOG_*.txt` — project documentation/history from upstream

## User preferences
None recorded yet.
