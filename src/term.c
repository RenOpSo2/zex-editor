/**
 * term.c - Terminal handling module
 * 
 * Provides low-level terminal control including:
 * - Raw input mode (no echo, no line buffering)
 * - Alternate screen buffer
 * - Mouse event tracking
 * - Window size management
 */

#include "term.h"
#include "config.h"
#include <stdint.h>
#include <sys/ioctl.h>
#include <unistd.h>

/* Terminal control flags (from termios.h) */
#define ICANON 0000002     /* Canonical mode (line buffering) */
#define ECHO   0000010     /* Echo input characters */
#define VMIN   6           /* Index for MIN value in c_cc array */
#define VTIME  5           /* Index for TIME value in c_cc array */

/* IOCTL requests for terminal control */
#define TCGETS 0x5401      /* Get terminal attributes */
#define TCSETS 0x5402      /* Set terminal attributes */

/* ANSI escape sequences */
#define ALT_SCREEN_ON  "\x1b[?1049h"  /* Switch to alternate screen buffer */
#define RESET_ATTR     "\x1b[0m"      /* Reset all text attributes */

#define MOUSE_ON       "\x1b[?1000h"  /* Enable X10 mouse click tracking */
#define MOUSE_OFF      "\x1b[?1000l"  /* Disable mouse tracking */


/**
 * struct termios - Simplified terminal I/O settings structure
 * 
 * This mimics the real termios structure but uses simplified types
 * to avoid including the full termios.h header.
 */
struct termios {
    unsigned int c_iflag;      /* Input mode flags */
    unsigned int c_oflag;      /* Output mode flags */
    unsigned int c_cflag;      /* Control mode flags */
    unsigned int c_lflag;      /* Local mode flags */
    unsigned char c_line;      /* Line discipline */
    unsigned char c_cc[32];    /* Control characters (VMIN, VTIME, etc.) */
};


/**
 * term_read - Read input from terminal with timeout
 * @dst: Destination buffer to store read data
 * 
 * Returns: Number of bytes read (0 on timeout, >0 on input)
 * 
 * Note: Uses term_capacity (global) to limit bytes read.
 *       Returns immediately if no input available (non-blocking with 100ms timeout).
 */
uint32_t term_read(char* dst)
{
    return read(STDIN_FILENO, dst, term_capacity);
}


/**
 * term_update - Update terminal window size information
 * @term: Terminal structure containing winsize to update
 * 
 * Fetches current terminal dimensions via ioctl TIOCGWINSZ.
 * Updates term->ws with current rows and columns.
 */
void term_update(struct term* term)
{
    ioctl(STDIN_FILENO, TIOCGWINSZ, &term->ws);
}


/**
 * term_deinit - Restore terminal to normal state
 * 
 * Cleanup function that should be called before program exit.
 * Performs:
 * 1. Disables mouse tracking
 * 2. Resets colors to default
 * 3. Exits alternate screen buffer
 * 4. Restores canonical mode with echo
 * 
 * This ensures the terminal returns to a usable state.
 */
void term_deinit()
{
    /* Always disable mouse tracking on exit */
    write(STDOUT_FILENO, MOUSE_OFF, sizeof(MOUSE_OFF) - 1);

    /* Reset colors to default */
    write(STDOUT_FILENO, "\x1b[0m\x1b[39;49m", sizeof("\x1b[0m\x1b[39;49m") - 1);
    /* Exit alternate screen buffer */
    write(STDOUT_FILENO, "\x1b[?1049l", sizeof("\x1b[?1049l") - 1);
    
    /* Restore original terminal settings (canonical + echo) */
    struct termios term;
    ioctl(STDIN_FILENO, TCGETS, &term);
    term.c_lflag |= (ICANON | ECHO);  /* Re-enable line buffering and echo */
    ioctl(STDIN_FILENO, TCSETS, &term);
}


/**
 * term_init - Initialize terminal for raw input mode
 * 
 * Sets up the terminal for interactive applications:
 * 1. Disables canonical mode (no line buffering)
 * 2. Disables echo (hide typed characters)
 * 3. Sets read() to return immediately with 100ms timeout
 * 4. Switches to alternate screen buffer
 * 5. Hides cursor
 * 6. Enables mouse tracking (if configured)
 * 
 * This should be called once at program startup.
 */
void term_init(void)
{
    struct termios term;

    /* Get current terminal attributes */
    if (ioctl(STDIN_FILENO, TCGETS, &term) == -1)
        return;  /* Failed to get terminal settings, skip setup */

    /* Disable canonical mode and echo for raw input */
    term.c_lflag &= ~(ICANON | ECHO);
    
    /* Set control characters for non-blocking read:
     * VMIN=0  -> read() returns immediately even with 0 bytes
     * VTIME=1 -> timeout of 100ms (tenths of seconds)
     */
    term.c_cc[VMIN]  = 0;   /* Return immediately, even with no bytes */
    term.c_cc[VTIME] = 1;   /* 100ms timeout for read() */

    /* Apply modified terminal settings */
    ioctl(STDIN_FILENO, TCSETS, &term);

    /* Enter alternate screen, reset colors to default, hide cursor */
    write(STDOUT_FILENO, ALT_SCREEN_ON, sizeof(ALT_SCREEN_ON) - 1);
    write(STDOUT_FILENO, RESET_ATTR, sizeof(RESET_ATTR) - 1);
    write(STDOUT_FILENO, "\x1b[?25l", sizeof("\x1b[?25l") - 1);  /* Hide cursor */

    /* Enable mouse tracking if configured */
    if (config_get_bool("mouse", 1)) {
        write(STDOUT_FILENO, MOUSE_ON, sizeof(MOUSE_ON) - 1);
    }
}
