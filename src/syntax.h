#ifndef SYNTAX_H
#define SYNTAX_H

#include "global.h"
#include <stdint.h>

// Initialize Python interpreter and syntax highlighting module
void syntax_init(void);

// Cleanup Python interpreter
void syntax_deinit(void);

// Highlight a single line of C code
// Returns a heap-allocated string that must be freed by the caller
char* syntax_highlight_line(const char* line, uint32_t line_len);

// Highlight a chunk of C code (multiple lines)
// Returns a heap-allocated string that must be freed by the caller
char* syntax_highlight_chunk(const char* code, uint32_t code_len);

// Highlight a single line of Python code
// Returns a heap-allocated string that must be freed by the caller
char* syntax_highlight_python_line(const char* line, uint32_t line_len);

// Highlight a chunk of Python code (multiple lines)
// Returns a heap-allocated string that must be freed by the caller
char* syntax_highlight_python_chunk(const char* code, uint32_t code_len);

// Get syntax highlighting function based on file extension
// Returns 1 for C/C++, 2 for Python, 0 for no highlighting
int syntax_get_language(const char* filepath);

// Check if syntax highlighting is available
int syntax_is_available(void);

#endif