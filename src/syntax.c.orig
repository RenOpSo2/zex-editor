#include "syntax.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <strings.h>

// ANSI color codes - ALL GREEN as requested
#define COLOR_RESET "\033[0m"
#define COLOR_KEYWORD "\033[38;5;2m"       // Green
#define COLOR_TYPE "\033[38;5;28m"         // Green (lighter)
#define COLOR_FUNCTION "\033[38;5;34m"    // Green (bright)
#define COLOR_STRING "\033[38;5;71m"       // Green (brightest)
#define COLOR_COMMENT "\033[38;5;242m"     // Green (dark gray)
#define COLOR_NUMBER "\033[38;5;46m"       // Green (intense)
#define COLOR_OPERATOR "\033[38;5;82m"     // Green (very bright)
#define COLOR_PREPROC "\033[38;5;106m"     // Green (medium)
#define COLOR_CONSTANT "\033[38;5;120m"    // Green (light)

// C/C++ keywords
static const char* keywords[] = {
    "break", "case", "continue", "default", "do", "else", "for", "goto",
    "if", "return", "switch", "while", "sizeof", "typedef", "struct",
    "union", "enum", "extern", "static", "const", "volatile", "register",
    "inline", "restrict", "auto", "void",
    // C++ keywords
    "class", "namespace", "template", "typename", "using", "public",
    "private", "protected", "virtual", "friend", "this", "new", "delete",
    "try", "catch", "throw", "const_cast", "dynamic_cast", "reinterpret_cast",
    "static_cast", "typeid", "explicit", "mutable", "operator", "and", "or",
    "not", "xor", "and_eq", "or_eq", "not_eq", "xor_eq", "noexcept",
    "nullptr", "override", "final", "constexpr", "decltype", "static_assert",
    "thread_local", "concept", "requires", "co_await", "co_return", "co_yield",
    "decltype", "module", "import", "export", NULL
};

// C/C++ types
static const char* types[] = {
    "int", "char", "float", "double", "long", "short", "signed",
    "unsigned", "bool", "_Bool", "complex", "_Complex", "imaginary", "_Imaginary",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t",
    "size_t", "ssize_t", "FILE",
    // C++ types
    "string", "wstring", "vector", "list", "map", "set", "unordered_map",
    "unordered_set", "array", "deque", "queue", "stack", "pair", "tuple",
    "shared_ptr", "unique_ptr", "weak_ptr", "optional", "variant", "any",
    "function", "iterator", "const_iterator", "reverse_iterator", "string_view",
    "ostream", "istream", "iostream", "fstream", "stringstream", "regex",
    "thread", "mutex", "lock_guard", "unique_lock", "condition_variable",
    "future", "promise", "atomic", "chrono", "time_point", "duration", NULL
};

// C constants
static const char* constants[] = {
    "NULL", "true", "false", "TRUE", "FALSE", "EOF", "stdin", "stdout", "stderr", NULL
};

// Python keywords
static const char* python_keywords[] = {
    "False", "None", "True", "and", "as", "assert", "async", "await",
    "break", "class", "continue", "def", "del", "elif", "else", "except",
    "finally", "for", "from", "global", "if", "import", "in", "is",
    "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
    "while", "with", "yield", NULL
};

// Python types
static const char* python_types[] = {
    "int", "float", "str", "bool", "list", "tuple", "dict", "set",
    "frozenset", "bytes", "bytearray", "complex", "range", "type", "object",
    "None", "Ellipsis", "NotImplemented", "function", "module", "property",
    "classmethod", "staticmethod", "super", "Exception", "ValueError", "TypeError",
    "KeyError", "IndexError", "RuntimeError", "IOError", "OSError", NULL
};

// Python constants
static const char* python_constants[] = {
    "True", "False", "None", "__name__", "__file__", "__doc__", "__package__",
    "self", "cls", "args", "kwargs", NULL
};

static bool is_keyword(const char* word, int len) {
    for (int i = 0; keywords[i] != NULL; i++) {
        size_t kw_len = strlen(keywords[i]);
        if (kw_len == (size_t)len && strncmp(word, keywords[i], len) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_type(const char* word, int len) {
    for (int i = 0; types[i] != NULL; i++) {
        size_t type_len = strlen(types[i]);
        if (type_len == (size_t)len && strncmp(word, types[i], len) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_constant(const char* word, int len) {
    for (int i = 0; constants[i] != NULL; i++) {
        size_t const_len = strlen(constants[i]);
        if (const_len == (size_t)len && strncmp(word, constants[i], len) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_python_keyword(const char* word, int len) {
    for (int i = 0; python_keywords[i] != NULL; i++) {
        size_t kw_len = strlen(python_keywords[i]);
        if (kw_len == (size_t)len && strncmp(word, python_keywords[i], len) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_python_type(const char* word, int len) {
    for (int i = 0; python_types[i] != NULL; i++) {
        size_t type_len = strlen(python_types[i]);
        if (type_len == (size_t)len && strncmp(word, python_types[i], len) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_python_constant(const char* word, int len) {
    for (int i = 0; python_constants[i] != NULL; i++) {
        size_t const_len = strlen(python_constants[i]);
        if (const_len == (size_t)len && strncmp(word, python_constants[i], len) == 0) {
            return true;
        }
    }
    return false;
}

static bool is_operator(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || 
           c == '=' || c == '&' || c == '|' || c == '^' || c == '~' || 
           c == '<' || c == '>' || c == '!' || c == '?' || c == ':';
}

static bool is_two_char_op(const char* s) {
    return (s[0] == '+' && s[1] == '+') ||
           (s[0] == '-' && s[1] == '-') ||
           (s[0] == '<' && s[1] == '<') ||
           (s[0] == '>' && s[1] == '>') ||
           (s[0] == '<' && s[1] == '=') ||
           (s[0] == '>' && s[1] == '=') ||
           (s[0] == '=' && s[1] == '=') ||
           (s[0] == '!' && s[1] == '=') ||
           (s[0] == '&' && s[1] == '&') ||
           (s[0] == '|' && s[1] == '|') ||
           (s[0] == '-' && s[1] == '>') ||
           (s[0] == '+' && s[1] == '=') ||
           (s[0] == '-' && s[1] == '=') ||
           (s[0] == '*' && s[1] == '=') ||
           (s[0] == '/' && s[1] == '=') ||
           (s[0] == '%' && s[1] == '=') ||
           (s[0] == '&' && s[1] == '=') ||
           (s[0] == '|' && s[1] == '=') ||
           (s[0] == '^' && s[1] == '=');
}

static void append_color(char** result, int* result_len, int* result_cap, const char* color) {
    int color_len = strlen(color);
    while (*result_len + color_len >= *result_cap) {
        *result_cap *= 2;
        *result = realloc(*result, *result_cap);
    }
    strcpy(*result + *result_len, color);
    *result_len += color_len;
}

static void append_text(char** result, int* result_len, int* result_cap, const char* text, int text_len) {
    while (*result_len + text_len >= *result_cap) {
        *result_cap *= 2;
        *result = realloc(*result, *result_cap);
    }
    memcpy(*result + *result_len, text, text_len);
    *result_len += text_len;
}

static void append_char(char** result, int* result_len, int* result_cap, char c) {
    while (*result_len + 1 >= *result_cap) {
        *result_cap *= 2;
        *result = realloc(*result, *result_cap);
    }
    (*result)[*result_len] = c;
    *result_len += 1;
}

char* syntax_highlight_python_line(const char* line, uint32_t line_len) {
    if (line_len == 0) {
        char* result = malloc(1);
        if (result) result[0] = '\0';
        return result;
    }

    int result_cap = line_len * 4;
    char* result = malloc(result_cap);
    if (!result) return NULL;
    
    int result_len = 0;
    int i = 0;
    
    while (i < (int)line_len) {
        // Skip ANSI escape sequences
        if (line[i] == '\033') {
            int end = i;
            while (end < (int)line_len && line[end] != 'm') {
                end++;
            }
            if (end < (int)line_len) {
                append_text(&result, &result_len, &result_cap, line + i, end - i + 1);
                i = end + 1;
                continue;
            }
        }
        
        // Check for Python comment (#)
        if (line[i] == '#') {
            append_color(&result, &result_len, &result_cap, COLOR_COMMENT);
            append_text(&result, &result_len, &result_cap, line + i, line_len - i);
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            i = line_len;
            continue;
        }
        
        // Check for string literal (double quote)
        if (line[i] == '"') {
            append_color(&result, &result_len, &result_cap, COLOR_STRING);
            append_char(&result, &result_len, &result_cap, '"');
            i++;
            
            // Check for triple quotes
            if (i + 2 < (int)line_len && line[i] == '"' && line[i+1] == '"') {
                append_char(&result, &result_len, &result_cap, '"');
                append_char(&result, &result_len, &result_cap, '"');
                i += 2;
                while (i < (int)line_len && !(line[i] == '"' && line[i+1] == '"' && line[i+2] == '"')) {
                    if (line[i] == '\\' && i + 1 < (int)line_len) {
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                    } else {
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                    }
                }
                if (i + 2 < (int)line_len) {
                    append_char(&result, &result_len, &result_cap, '"');
                    append_char(&result, &result_len, &result_cap, '"');
                    append_char(&result, &result_len, &result_cap, '"');
                    i += 3;
                }
            } else {
                while (i < (int)line_len && line[i] != '"') {
                    if (line[i] == '\\' && i + 1 < (int)line_len) {
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                    } else {
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                    }
                }
                if (i < (int)line_len) {
                    append_char(&result, &result_len, &result_cap, '"');
                    i++;
                }
            }
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            continue;
        }
        
        // Check for string literal (single quote)
        if (line[i] == '\'') {
            append_color(&result, &result_len, &result_cap, COLOR_STRING);
            append_char(&result, &result_len, &result_cap, '\'');
            i++;
            
            // Check for triple quotes
            if (i + 2 < (int)line_len && line[i] == '\'' && line[i+1] == '\'') {
                append_char(&result, &result_len, &result_cap, '\'');
                append_char(&result, &result_len, &result_cap, '\'');
                i += 2;
                while (i < (int)line_len && !(line[i] == '\'' && line[i+1] == '\'' && line[i+2] == '\'')) {
                    if (line[i] == '\\' && i + 1 < (int)line_len) {
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                    } else {
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                    }
                }
                if (i + 2 < (int)line_len) {
                    append_char(&result, &result_len, &result_cap, '\'');
                    append_char(&result, &result_len, &result_cap, '\'');
                    append_char(&result, &result_len, &result_cap, '\'');
                    i += 3;
                }
            } else {
                while (i < (int)line_len && line[i] != '\'') {
                    if (line[i] == '\\' && i + 1 < (int)line_len) {
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                    } else {
                        append_char(&result, &result_len, &result_cap, line[i]);
                        i++;
                    }
                }
                if (i < (int)line_len) {
                    append_char(&result, &result_len, &result_cap, '\'');
                    i++;
                }
            }
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            continue;
        }
        
        // Check for identifier
        if (isalpha(line[i]) || line[i] == '_') {
            int start = i;
            while (i < (int)line_len && (isalnum(line[i]) || line[i] == '_')) {
                i++;
            }
            
            int word_len = i - start;
            
            // Check if it's a Python keyword
            if (is_python_keyword(line + start, word_len)) {
                append_color(&result, &result_len, &result_cap, COLOR_KEYWORD);
                append_text(&result, &result_len, &result_cap, line + start, word_len);
                append_color(&result, &result_len, &result_cap, COLOR_RESET);
            }
            // Check if it's a Python type
            else if (is_python_type(line + start, word_len)) {
                append_color(&result, &result_len, &result_cap, COLOR_TYPE);
                append_text(&result, &result_len, &result_cap, line + start, word_len);
                append_color(&result, &result_len, &result_cap, COLOR_RESET);
            }
            // Check if it's a Python constant
            else if (is_python_constant(line + start, word_len)) {
                append_color(&result, &result_len, &result_cap, COLOR_CONSTANT);
                append_text(&result, &result_len, &result_cap, line + start, word_len);
                append_color(&result, &result_len, &result_cap, COLOR_RESET);
            }
            // Check if it's a function call
            else {
                int j = i;
                while (j < (int)line_len && isspace(line[j])) {
                    j++;
                }
                if (j < (int)line_len && line[j] == '(') {
                    append_color(&result, &result_len, &result_cap, COLOR_FUNCTION);
                    append_text(&result, &result_len, &result_cap, line + start, word_len);
                    append_color(&result, &result_len, &result_cap, COLOR_RESET);
                } else {
                    append_text(&result, &result_len, &result_cap, line + start, word_len);
                }
            }
            continue;
        }
        
        // Check for number
        if (isdigit(line[i]) || (line[i] == '.' && i + 1 < (int)line_len && isdigit(line[i+1]))) {
            int start = i;
            append_color(&result, &result_len, &result_cap, COLOR_NUMBER);
            
            if (line[i] == '0' && i + 1 < (int)line_len && tolower(line[i+1]) == 'x') {
                i += 2;
                while (i < (int)line_len && (isxdigit(line[i]))) {
                    i++;
                }
            } else if (line[i] == '0' && i + 1 < (int)line_len && tolower(line[i+1]) == 'b') {
                i += 2;
                while (i < (int)line_len && (line[i] == '0' || line[i] == '1')) {
                    i++;
                }
            } else if (line[i] == '0' && i + 1 < (int)line_len && tolower(line[i+1]) == 'o') {
                i += 2;
                while (i < (int)line_len && (line[i] >= '0' && line[i] <= '7')) {
                    i++;
                }
            } else {
                while (i < (int)line_len && (isdigit(line[i]) || line[i] == '.')) {
                    i++;
                }
                if (i < (int)line_len && tolower(line[i]) == 'e') {
                    i++;
                    if (i < (int)line_len && (line[i] == '+' || line[i] == '-')) {
                        i++;
                    }
                    while (i < (int)line_len && isdigit(line[i])) {
                        i++;
                    }
                }
                if (i < (int)line_len && tolower(line[i]) == 'j') {
                    i++;
                }
            }
            
            append_text(&result, &result_len, &result_cap, line + start, i - start);
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            continue;
        }
        
        // Check for operator
        if (is_operator(line[i])) {
            append_color(&result, &result_len, &result_cap, COLOR_OPERATOR);
            
            if (i + 1 < (int)line_len && is_two_char_op(line + i)) {
                append_char(&result, &result_len, &result_cap, line[i]);
                append_char(&result, &result_len, &result_cap, line[i+1]);
                i += 2;
            } else {
                append_char(&result, &result_len, &result_cap, line[i]);
                i++;
            }
            
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            continue;
        }
        
        // Default: append character as-is
        append_char(&result, &result_len, &result_cap, line[i]);
        i++;
    }
    
    // Null-terminate
    append_char(&result, &result_len, &result_cap, '\0');
    
    return result;
}

char* syntax_highlight_line(const char* line, uint32_t line_len) {
    if (line_len == 0) {
        char* result = malloc(1);
        if (result) result[0] = '\0';
        return result;
    }

    int result_cap = line_len * 4; // Estimate for color codes
    char* result = malloc(result_cap);
    if (!result) return NULL;
    
    int result_len = 0;
    int i = 0;
    
    while (i < (int)line_len) {
        // Skip ANSI escape sequences
        if (line[i] == '\033') {
            int end = i;
            while (end < (int)line_len && line[end] != 'm') {
                end++;
            }
            if (end < (int)line_len) {
                append_text(&result, &result_len, &result_cap, line + i, end - i + 1);
                i = end + 1;
                continue;
            }
        }
        
        // Check for multi-line comment start
        if (i + 1 < (int)line_len && line[i] == '/' && line[i+1] == '*') {
            append_color(&result, &result_len, &result_cap, COLOR_COMMENT);
            append_char(&result, &result_len, &result_cap, '/');
            append_char(&result, &result_len, &result_cap, '*');
            i += 2;
            
            while (i < (int)line_len) {
                if (i + 1 < (int)line_len && line[i] == '*' && line[i+1] == '/') {
                    append_char(&result, &result_len, &result_cap, '*');
                    append_char(&result, &result_len, &result_cap, '/');
                    i += 2;
                    break;
                }
                append_char(&result, &result_len, &result_cap, line[i]);
                i++;
            }
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            continue;
        }
        
        // Check for single-line comment
        if (i + 1 < (int)line_len && line[i] == '/' && line[i+1] == '/') {
            append_color(&result, &result_len, &result_cap, COLOR_COMMENT);
            append_text(&result, &result_len, &result_cap, line + i, line_len - i);
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            i = line_len;
            continue;
        }
        
        // Check for preprocessor directive
        if (line[i] == '#') {
            append_color(&result, &result_len, &result_cap, COLOR_PREPROC);
            while (i < (int)line_len && line[i] != '\n') {
                append_char(&result, &result_len, &result_cap, line[i]);
                i++;
            }
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            continue;
        }
        
        // Check for string literal
        if (line[i] == '"') {
            append_color(&result, &result_len, &result_cap, COLOR_STRING);
            append_char(&result, &result_len, &result_cap, '"');
            i++;
            
            while (i < (int)line_len && line[i] != '"') {
                if (line[i] == '\\' && i + 1 < (int)line_len) {
                    append_char(&result, &result_len, &result_cap, line[i]);
                    i++;
                    append_char(&result, &result_len, &result_cap, line[i]);
                    i++;
                } else {
                    append_char(&result, &result_len, &result_cap, line[i]);
                    i++;
                }
            }
            
            if (i < (int)line_len) {
                append_char(&result, &result_len, &result_cap, '"');
                i++;
            }
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            continue;
        }
        
        // Check for character literal
        if (line[i] == '\'') {
            append_color(&result, &result_len, &result_cap, COLOR_STRING);
            append_char(&result, &result_len, &result_cap, '\'');
            i++;
            
            while (i < (int)line_len && line[i] != '\'') {
                if (line[i] == '\\' && i + 1 < (int)line_len) {
                    append_char(&result, &result_len, &result_cap, line[i]);
                    i++;
                    append_char(&result, &result_len, &result_cap, line[i]);
                    i++;
                } else {
                    append_char(&result, &result_len, &result_cap, line[i]);
                    i++;
                }
            }
            
            if (i < (int)line_len) {
                append_char(&result, &result_len, &result_cap, '\'');
                i++;
            }
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            continue;
        }
        
        // Check for identifier
        if (isalpha(line[i]) || line[i] == '_') {
            int start = i;
            while (i < (int)line_len && (isalnum(line[i]) || line[i] == '_')) {
                i++;
            }
            
            int word_len = i - start;
            
            // Check if it's a keyword
            if (is_keyword(line + start, word_len)) {
                append_color(&result, &result_len, &result_cap, COLOR_KEYWORD);
                append_text(&result, &result_len, &result_cap, line + start, word_len);
                append_color(&result, &result_len, &result_cap, COLOR_RESET);
            }
            // Check if it's a type
            else if (is_type(line + start, word_len)) {
                append_color(&result, &result_len, &result_cap, COLOR_TYPE);
                append_text(&result, &result_len, &result_cap, line + start, word_len);
                append_color(&result, &result_len, &result_cap, COLOR_RESET);
            }
            // Check if it's a constant
            else if (is_constant(line + start, word_len)) {
                append_color(&result, &result_len, &result_cap, COLOR_CONSTANT);
                append_text(&result, &result_len, &result_cap, line + start, word_len);
                append_color(&result, &result_len, &result_cap, COLOR_RESET);
            }
            // Check if it's a function call
            else {
                int j = i;
                while (j < (int)line_len && isspace(line[j])) {
                    j++;
                }
                if (j < (int)line_len && line[j] == '(') {
                    append_color(&result, &result_len, &result_cap, COLOR_FUNCTION);
                    append_text(&result, &result_len, &result_cap, line + start, word_len);
                    append_color(&result, &result_len, &result_cap, COLOR_RESET);
                } else {
                    append_text(&result, &result_len, &result_cap, line + start, word_len);
                }
            }
            continue;
        }
        
        // Check for number
        if (isdigit(line[i]) || (line[i] == '.' && i + 1 < (int)line_len && isdigit(line[i+1]))) {
            int start = i;
            append_color(&result, &result_len, &result_cap, COLOR_NUMBER);
            
            if (line[i] == '0' && i + 1 < (int)line_len && tolower(line[i+1]) == 'x') {
                i += 2;
                while (i < (int)line_len && (isxdigit(line[i]))) {
                    i++;
                }
            } else {
                while (i < (int)line_len && (isdigit(line[i]) || line[i] == '.')) {
                    i++;
                }
                if (i < (int)line_len && tolower(line[i]) == 'e') {
                    i++;
                    if (i < (int)line_len && (line[i] == '+' || line[i] == '-')) {
                        i++;
                    }
                    while (i < (int)line_len && isdigit(line[i])) {
                        i++;
                    }
                }
                if (i < (int)line_len && tolower(line[i]) == 'f') {
                    i++;
                }
            }
            
            append_text(&result, &result_len, &result_cap, line + start, i - start);
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            continue;
        }
        
        // Check for operator
        if (is_operator(line[i])) {
            append_color(&result, &result_len, &result_cap, COLOR_OPERATOR);
            
            if (i + 1 < (int)line_len && is_two_char_op(line + i)) {
                append_char(&result, &result_len, &result_cap, line[i]);
                append_char(&result, &result_len, &result_cap, line[i+1]);
                i += 2;
            } else {
                append_char(&result, &result_len, &result_cap, line[i]);
                i++;
            }
            
            append_color(&result, &result_len, &result_cap, COLOR_RESET);
            continue;
        }
        
        // Default: append character as-is
        append_char(&result, &result_len, &result_cap, line[i]);
        i++;
    }
    
    // Null-terminate
    append_char(&result, &result_len, &result_cap, '\0');
    
    return result;
}

char* syntax_highlight_chunk(const char* code, uint32_t code_len) {
    // For chunks, we'll process line by line
    if (code_len == 0) {
        char* result = malloc(1);
        if (result) result[0] = '\0';
        return result;
    }

    int result_cap = code_len * 4;
    char* result = malloc(result_cap);
    if (!result) return NULL;
    
    int result_len = 0;
    int line_start = 0;
    
    for (int i = 0; i <= (int)code_len; i++) {
        if (i == (int)code_len || code[i] == '\n') {
            int line_len = i - line_start;
            char* highlighted = syntax_highlight_line(code + line_start, line_len);
            if (highlighted) {
                append_text(&result, &result_len, &result_cap, highlighted, strlen(highlighted));
                free(highlighted);
            }
            if (i < (int)code_len) {
                append_char(&result, &result_len, &result_cap, '\n');
            }
            line_start = i + 1;
        }
    }
    
    append_char(&result, &result_len, &result_cap, '\0');
    return result;
}

char* syntax_highlight_python_chunk(const char* code, uint32_t code_len) {
    // For chunks, we'll process line by line
    if (code_len == 0) {
        char* result = malloc(1);
        if (result) result[0] = '\0';
        return result;
    }

    int result_cap = code_len * 4;
    char* result = malloc(result_cap);
    if (!result) return NULL;
    
    int result_len = 0;
    int line_start = 0;
    
    for (int i = 0; i <= (int)code_len; i++) {
        if (i == (int)code_len || code[i] == '\n') {
            int line_len = i - line_start;
            char* highlighted = syntax_highlight_python_line(code + line_start, line_len);
            if (highlighted) {
                append_text(&result, &result_len, &result_cap, highlighted, strlen(highlighted));
                free(highlighted);
            }
            if (i < (int)code_len) {
                append_char(&result, &result_len, &result_cap, '\n');
            }
            line_start = i + 1;
        }
    }
    
    append_char(&result, &result_len, &result_cap, '\0');
    return result;
}

int syntax_get_language(const char* filepath) {
    if (!filepath) {
        return 0;
    }
    
    const char* ext = strrchr(filepath, '.');
    if (!ext) {
        return 0;
    }
    
    ext++; // Skip the dot
    
    if (strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0) {
        return 1; // C
    }
    
    if (strcmp(ext, "cpp") == 0 || strcmp(ext, "cxx") == 0 || 
        strcmp(ext, "cc") == 0 || strcmp(ext, "hpp") == 0 || 
        strcmp(ext, "hxx") == 0) {
        return 1; // C++
    }
    
    if (strcmp(ext, "py") == 0 || strcmp(ext, "pyw") == 0) {
        return 2; // Python
    }
    
    if (strcmp(ext, "python") == 0) {
        return 2; // Python
    }
    
    return 0; // No highlighting
}

void syntax_init(void) {
    // No initialization needed for pure C implementation
}

void syntax_deinit(void) {
    // No cleanup needed for pure C implementation
}

int syntax_is_available(void) {
    return 1; // Always available in pure C implementation
}