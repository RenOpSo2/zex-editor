#include "term.h"
#include "config.h"

#include <cerrno>
#include <cstring>
#include <memory>
#include <sys/ioctl.h>
#include <unistd.h>

/* Terminal control flags (from termios.h) */
#define BRKINT 0000002     /* Signal on break condition */
#define ICRNL  0000400     /* Translate CR to NL on input */
#define IXON   0002000     /* XON/XOFF flow control (^S/^Q interception) */
#define OPOST  0000001     /* Output post-processing (ONLCR etc.) */
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

namespace {

static void write_literal(const char* s)
{
    if (!s) {
        return;
    }

    size_t len = std::strlen(s);
    size_t off = 0;
    while (off < len) {
        ssize_t n = ::write(STDOUT_FILENO, s + off, len - off);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (n == 0) {
            break;
        }
        off += static_cast<size_t>(n);
    }
}

class TerminalSession {
public:
    TerminalSession() = default;
    ~TerminalSession() { restore(); }

    TerminalSession(const TerminalSession&) = delete;
    TerminalSession& operator=(const TerminalSession&) = delete;

    bool init()
    {
        if (active_) {
            return true;
        }

        struct termios term{};

        /* Get current terminal attributes */
        if (::ioctl(STDIN_FILENO, TCGETS, &term) == -1) {
            return false;
        }

        original_ = term;
        raw_mode_active_ = true;

        /*
         * Input: no flow control (^S/^Q are editor bindings), no CR/NL
         * translation, no break/parity surprises.
         */
        term.c_iflag &= ~(unsigned int)(IXON | ICRNL | BRKINT);

        /* Output: no post-processing; the renderer writes explicit CRLF. */
        term.c_oflag &= ~(unsigned int)(OPOST);

        /* Local: raw input (no line buffering, no echo). */
        term.c_lflag &= ~(unsigned int)(ICANON | ECHO);

        /* Set control characters for blocking read:
         * VMIN=1  -> read() blocks until at least 1 byte is available
         * VTIME=0 -> no timeout (blocking)
         */
        term.c_cc[VMIN] = 1;
        term.c_cc[VTIME] = 0;

        /* Apply modified terminal settings */
        if (::ioctl(STDIN_FILENO, TCSETS, &term) == -1) {
            raw_mode_active_ = false;
            return false;
        }

        /* Enter alternate screen, reset colors to default, hide cursor */
        write_literal(ALT_SCREEN_ON);
        write_literal(RESET_ATTR);
        write_literal("\x1b[?25l");  /* Hide cursor */

        /* Enable mouse tracking if configured */
        if (config_get_bool("mouse", 1)) {
            write_literal(MOUSE_ON);
        }

        active_ = true;
        return true;
    }

    void restore()
    {
        if (!active_) {
            return;
        }

        /* Always disable mouse tracking on exit */
        write_literal(MOUSE_OFF);

        /* Reset colors to default */
        write_literal("\x1b[0m\x1b[39;49m");
        /* Exit alternate screen buffer */
        write_literal("\x1b[?1049l");

        /* Restore the original terminal settings captured at init. */
        if (raw_mode_active_) {
            ::ioctl(STDIN_FILENO, TCSETS, &original_);
            raw_mode_active_ = false;
        }

        active_ = false;
    }

private:
    struct termios original_{};
    bool raw_mode_active_ = false;
    bool active_ = false;
};

static std::unique_ptr<TerminalSession> g_session;

} // namespace

extern "C" {

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
    if (!dst) {
        return 0;
    }

    while (true) {
        ssize_t n = ::read(STDIN_FILENO, dst, term_capacity);
        if (n >= 0) {
            return static_cast<uint32_t>(n);
        }
        if (errno == EINTR) {
            continue;
        }
        return 0;
    }
}

/**
 * term_update - Update terminal window size information
 * @term: Terminal structure containing winsize to update
 *
 * Fetches current terminal dimensions via ioctl TIOCGWINSZ.
 * Updates term->ws with current rows and columns.
 *
 * Edge cases:
 * - If the ioctl fails (e.g. stdin is not a tty) or reports a zero
 *   dimension, the last known good size is kept; if there is none yet,
 *   a conventional 80x24 fallback is installed so downstream rendering
 *   never sees a zero-sized screen (which would underflow its maths).
 */
void term_update(struct term* term)
{
    if (!term) {
        return;
    }

    struct winsize ws;

    if (::ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
        term->ws = ws;
        return;
    }

    /* No usable report: fall back, but never clobber a known good size. */
    if (term->ws.ws_row == 0 || term->ws.ws_col == 0) {
        term->ws.ws_row = 24;
        term->ws.ws_col = 80;
    }
}

/**
 * term_init - Initialize terminal for raw input mode
 *
 * Sets up the terminal for interactive applications:
 * 1. Saves the current attributes so deinit can restore them exactly
 * 2. Disables canonical mode (no line buffering) and echo
 * 3. Disables ^S/^Q flow control so Ctrl+S/Ctrl+Q reach the editor
 * 4. Disables output post-processing: frames already carry explicit
 *    CR-LF, making rendering deterministic regardless of ONLCR
 * 5. Sets read() to return after a 100ms timeout
 * 6. Switches to alternate screen buffer
 * 7. Hides cursor
 * 8. Enables mouse tracking (if configured)
 *
 * This should be called once at program startup.
 */
void term_init(void)
{
    if (!g_session) {
        g_session = std::make_unique<TerminalSession>();
    }

    if (!g_session->init()) {
        g_session.reset();
    }
}

/**
 * term_deinit - Restore terminal to normal state
 *
 * Cleanup function that should be called before program exit.
 * Performs:
 * 1. Disables mouse tracking
 * 2. Resets colors to default
 * 3. Exits alternate screen buffer
 * 4. Restores the original terminal settings saved by term_init
 *
 * This ensures the terminal returns to exactly the state it was in
 * before the editor started.
 */
void term_deinit(void)
{
    g_session.reset();
}

} // extern "C"
