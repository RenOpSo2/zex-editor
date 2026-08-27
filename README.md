# Zex

Zex is a very efficient text editor for terminal users.

## Why use zex?

zex is considered very efficient for writing everyday code and is friendly for users using shortcuts like vscode.

## Features

- **Fast and efficient**: Built with C for maximum performance
- **Config System**: full kontrol
- **Auto-indent**: Automatically preserves indentation when pressing Enter
- **Undo/Redo**: Full undo/redo support with Ctrl+U and Ctrl+Y
- **Syntax highlighting**: C syntax highlighting for better code readability
- **Large file support**: Paged Gap Buffer for handling massive documents
- **Clipboard**: Copy, cut, and paste functionality
- **Selection**: Shift+arrow keys for text selection
- **Search**: Find text with Ctrl+F, navigate with Ctrl+N (next) / Ctrl+P (previous)
- **Line Numbers**: Display line numbers in left gutter
- **Screen Refresh**: Ctrl+R to fix rendering issues
- **Memory efficient**: Uses arena allocator for optimized memory management

## Installation

### Building from source

```bash
git clone <repository-url>
cd zex
make
```

The compiled binary will be available at `bin/zex`.

### Running

```bash
./bin/zex [filename]
```

Or use the make target:

```bash
make run
```

### Keybindings

#### Editing
- **Arrow keys**: Move cursor up/down/left/right
- **Enter**: Insert new line (with auto-indent)
- **Backspace/Delete**: Delete character
- **Tab**: Insert tab character

#### File Operations
- **Ctrl+S**: Save file
- **Ctrl+Q**: Quit editor

#### Edit Operations
- **Ctrl+C**: Copy selection
- **Ctrl+X**: Cut selection  
- **Ctrl+V**: Paste clipboard
- **Ctrl+A**: Select all

#### Undo/Redo
- **Ctrl+U**: Undo last action
- **Ctrl+Y**: Redo undone action

#### Search
- **Ctrl+F**: Start search
- **Ctrl+N**: Next match
- **Ctrl+P**: Previous match
- **ESC**: Cancel search

#### View
- **Ctrl+R**: Refresh screen

#### Selection
- **Shift+Arrow keys**: Extend selection

### Example Session

```bash
# Open a C file
./bin/zex example.c

# Edit your code (auto-indent works automatically)
# Use Ctrl+S to save
# Use Ctrl+Q to quit
```

## Architecture

- **Paged Gap Buffer**: Efficient text storage and manipulation
- **Arena Allocator**: Optimized memory management for nodes
- **Render Buffer**: Double-buffered terminal rendering
- **Syntax Highlighting**: Token-based C syntax highlighting
