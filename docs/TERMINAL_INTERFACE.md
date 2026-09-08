# Terminal Interface

The terminal interface handles raw mode terminal I/O, ANSI escape sequences, and mouse input for the Zex editor.

## Raw Mode

Raw mode disables terminal processing to allow direct character-by-character input.

### What Raw Mode Does

**Disables:**
- Line buffering (input sent immediately, not on Enter)
- Echo (characters not automatically displayed)
- Special character processing (Ctrl+C, Ctrl+Z, etc.)
- Output processing (automatic newline conversion)

**Enables:**
- Single-character input
- Special key detection (arrows, function keys)
- Mouse click events

### Raw Mode Setup

```c
struct termios {
    unsigned int c_iflag;      // Input mode flags
    unsigned int c_oflag;      // Output mode flags
    unsigned int c_cflag;      // Control mode flags
    unsigned int c_lflag;      // Local mode flags
    unsigned char c_line;      // Line discipline
    unsigned char c_cc[32];    // Control characters
};

void term_init(void)
{
    struct termios term;
    
    // Get current terminal attributes
    ioctl(STDIN_FILENO, TCGETS, &term);
    original_ = term;  // Save for restoration
    
    // Input: no flow control, no CR/NL translation
    term.c_iflag &= ~(IXON | ICRNL | BRKINT);
    
    // Output: no post-processing
    term.c_oflag &= ~(OPOST);
    
    // Local: raw mode (no echo, no canonical mode)
    term.c_lflag &= ~(ICANON | ECHO | ISIG);
    
    // Control chars: read 1 byte at a time (no timeout)
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    
    // Apply new settings
    ioctl(STDIN_FILENO, TCSETS, &term);
    
    // Enable alternate screen buffer
    write_literal(ALT_SCREEN_ON);
}
```

### Raw Mode Cleanup

```c
void term_deinit(void)
{
    // Restore original terminal settings
    ioctl(STDIN_FILENO, TCSETS, &original_);
    
    // Disable alternate screen buffer
    write_literal(ALT_SCREEN_DISABLE);
    
    // Reset attributes
    write_literal(RESET_ATTR);
}
```

## ANSI Escape Sequences

ANSI escape sequences control terminal appearance and cursor position.

### Common Sequences Used

**Cursor Movement:**
```c
#define ANSI_CURSOR_HOME "\x1b[H"        // Move to (1,1)
#define ANSI_CLEAR_SCREEN "\x1b[2J"      // Clear entire screen
#define ANSI_CLEAR_LINE "\x1b[K"         // Clear to end of line
```

**Text Attributes:**
```c
#define ANSI_RESET "\x1b[0m"             // Reset all attributes
#define ANSI_BOLD "\x1b[1m"              // Bold text
#define ANSI_DIM "\x1b[2m"               // Dim text
#define ANSI_BOLD_OFF "\x1b[22m"         // Disable bold/dim
```

**Colors:**
```c
#define ANSI_DEFAULT_FG "\x1b[39m"       // Default foreground
#define ANSI_DEFAULT_BG "\x1b[49m"       // Default background
#define ANSI_LN_COLOR "\x1b[38;5;238m"   // Line number color
#define ANSI_STATUS_BG "\x1b[48;5;238m"  // Status bar background
#define ANSI_STATUS_FG "\x1b[38;5;255m"  // Status bar foreground
```

**Cursor Visibility:**
```c
#define ANSI_CURSOR_SHOW "\x1b[?25h"     // Show cursor
#define ANSI_CURSOR_HIDE "\x1b[?25l"     // Hide cursor
```

**Alternate Screen:**
```c
#define ANSI_ALT_SCREEN_ENABLE "\x1b[?1049h"  // Enable alt screen
#define ANSI_ALT_SCREEN_DISABLE "\x1b[?1049l" // Disable alt screen
```

### Using Escape Sequences

```c
// Clear screen and position cursor
write_literal(ANSI_CLEAR_SCREEN);
write_literal(ANSI_CURSOR_HOME);

// Set color and print text
write_literal(ANSI_LN_COLOR);
write_literal(ANSI_BOLD);
printf("Line 1");
write_literal(ANSI_RESET);

// Move cursor to specific position
printf("\x1b[%d;%dH", row, col);
```

### Cursor Positioning

To move cursor to (row, col):

```c
printf("\x1b[%d;%dH", row, col);
```

Example: Move to row 5, column 10:
```c
printf("\x1b[5;10H");
```

## Mouse Handling

Zex uses the X10 mouse protocol for click events.

### Enabling Mouse Tracking

```c
#define MOUSE_ON "\x1b[?1000h"  // Enable X10 mouse tracking
#define MOUSE_OFF "\x1b[?1000l" // Disable mouse tracking

// Enable mouse when config allows
if (config_get_bool("mouse", 1)) {
    write_literal(MOUSE_ON);
}
```

### Mouse Event Format

X10 mouse events arrive as escape sequences:

```
\x1b[M <button> <x> <y>
```

- `<button>`: Button code + modifiers
- `<x>`: Column (32 + actual column)
- `<y>`: Row (32 + actual row)

### Parsing Mouse Events

```c
typedef enum {
    esc_none,
    esc_got_esc,     // received \x1b
    esc_got_bracket, // received \x1b[
    esc_got_M,       // received \x1b[M
    esc_got_M_btn,   // got button byte
    esc_got_M_x,     // got x byte
} esc_state;

// In input handler
if (ch == '\x1b') {
    esc = esc_got_esc;
} else if (esc == esc_got_M && ch == 'M') {
    esc = esc_got_M_btn;
} else if (esc == esc_got_M_btn) {
    mouse_btn_byte = ch;
    esc = esc_got_M_x;
} else if (esc == esc_got_M_x) {
    mouse_x_byte = ch;
    esc = esc_got_M_y;
} else if (esc == esc_got_M_y) {
    mouse_y_byte = ch;
    // Calculate actual position
    uint32_t screen_col = mouse_x_byte - 32;
    uint32_t screen_row = mouse_y_byte - 32;
    
    // Handle click
    mouse_click_move(global, screen_col, screen_row);
    
    esc = esc_none;
}
```

### Mouse Click to Cursor Position

Convert screen coordinates to buffer position:

```c
static void mouse_click_move(struct global* global, 
                              uint32_t screen_col, 
                              uint32_t screen_row)
{
    // Row 1 = status bar, Row 2+ = text area
    if (screen_row < 2) return;
    
    uint32_t scroll_offset = draw_get_scroll_offset();
    uint32_t target_line = (screen_row - 2) + scroll_offset;
    
    // Calculate column (account for gutter)
    int gutter = (int)draw_gutter_width();
    int target_col = (int)screen_col - 1 - gutter;
    if (target_col < 0) target_col = 0;
    
    // Find byte offset for (target_line, target_col)
    // ... walk buffer to find position
    
    pgb_move_to_pos(&global->text, pos);
}
```

## Terminal Size Detection

Get terminal dimensions:

```c
struct term {
    struct winsize ws;  // Window size
};

void term_update(struct term* term)
{
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &term->ws);
}

// Access dimensions
uint32_t rows = term.ws.ws_row;
uint32_t cols = term.ws.ws_col;
```

## Special Key Detection

Special keys (arrows, function keys) arrive as escape sequences.

### Arrow Keys

```
Up:    \x1b[A
Down:  \x1b[B
Right: \x1b[C
Left:  \x1b[D
```

### Shift+Arrow Keys

```
Shift+Up:    \x1b[1;2A
Shift+Down:  \x1b[1;2B
Shift+Right: \x1b[1;2C
Shift+Left:  \x1b[1;2D
```

### Parsing Arrow Keys

```c
static void handle_plain_arrow(struct global* global, char direction)
{
    switch (direction) {
        case 'A': // Up
            pgb_move_up(&global->text);
            break;
        case 'B': // Down
            pgb_move_down(&global->text);
            break;
        case 'C': // Right
            pgb_move_right(&global->text);
            break;
        case 'D': // Left
            pgb_move_left(&global->text);
            break;
    }
}

static void handle_shift_arrow(struct global* global, char direction)
{
    // Extend selection while moving
    switch (direction) {
        case 'A': // Shift+Up
            pgb_move_up(&global->text);
            sel_begin(global);
            break;
        // ... other directions
    }
}
```

## Terminal Input Reading

Read input character by character:

```c
uint32_t term_read(char* dst)
{
    ssize_t n = read(STDIN_FILENO, dst, 1);
    if (n <= 0) return 0;
    return 1;
}
```

## Safe Write Function

Write to terminal with error handling:

```c
static void write_literal(const char* s)
{
    if (!s) return;
    
    size_t len = strlen(s);
    size_t off = 0;
    
    while (off < len) {
        ssize_t n = write(STDOUT_FILENO, s + off, len - off);
        if (n < 0) {
            if (errno == EINTR) continue;  // Interrupted, retry
            break;  // Real error
        }
        if (n == 0) break;  // EOF
        off += (size_t)n;
    }
}
```

## Terminal Session Management

The terminal interface uses RAII-style management:

```c
class TerminalSession {
public:
    TerminalSession() = default;
    ~TerminalSession() { restore(); }  // Auto-restore on scope exit
    
    bool init() {
        if (active_) return true;
        
        // Save original settings
        ioctl(STDIN_FILENO, TCGETS, &original_);
        raw_mode_active_ = true;
        
        // Apply raw mode
        // ... termios modifications
        
        active_ = true;
        return true;
    }
    
    void restore() {
        if (!raw_mode_active_) return;
        
        // Restore original settings
        ioctl(STDIN_FILENO, TCSETS, &original_);
        raw_mode_active_ = false;
    }
    
private:
    struct termios original_;
    bool raw_mode_active_ = false;
    bool active_ = false;
};
```

## Platform Considerations

### Linux/Unix

- Uses `ioctl()` for terminal control
- Standard ANSI escape sequences
- X10 mouse protocol supported

### Terminal Compatibility

The editor works with:
- xterm
- gnome-terminal
- konsole
- iTerm2 (macOS)
- Terminal.app (macOS)
- Most modern terminal emulators

### Fallback Values

If terminal size detection fails:

```c
#define DRAW_FALLBACK_ROWS 24
#define DRAW_FALLBACK_COLS 80

uint32_t rows = term.ws.ws_row ? term.ws.ws_row : DRAW_FALLBACK_ROWS;
uint32_t cols = term.ws.ws_col ? term.ws.ws_col : DRAW_FALLBACK_COLS;
```

## Common Issues

### Terminal Not Resetting

If the editor crashes, terminal may remain in raw mode. Fix:

```bash
reset
# or
stty sane
```

### Mouse Not Working

Ensure:
- Mouse support enabled in config: `"mouse": true`
- Terminal supports X10 protocol
- Not running over SSH without proper terminal support

### Colors Not Displaying

Ensure:
- Terminal supports 256-color mode
- Not running in limited terminal (e.g., older console)

## Testing Terminal Interface

```bash
# Manual test of raw mode
./bin/zex test.c

# Test mouse support
./bin/zex test.c --mouse true

# Test alternate screen
# Editor should switch to alternate screen on start
# and restore on exit
```

## Best Practices

1. **Always restore terminal** - Use RAII or ensure cleanup on exit
2. **Handle write errors** - Check return values from write()
3. **Use safe I/O** - Handle EINTR and partial writes
4. **Test on multiple terminals** - Ensure compatibility
5. **Provide fallbacks** - Handle terminal detection failures