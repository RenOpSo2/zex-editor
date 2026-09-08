# Syntax Highlighting

Zex provides syntax highlighting for C/C++ and Python through a token-based highlighting system that categorizes keywords, types, constants, operators, and other language elements.

## Supported Languages

- **C/C++** - Full support with keywords, types, constants
- **Python** - Keywords, types, constants

## Token Categories

### Keywords

Reserved words that control program flow:

**C/C++ Keywords:**
```c
"break", "case", "continue", "default", "do", "else", "for", "goto",
"if", "return", "switch", "while", "sizeof", "typedef", "struct",
"union", "enum", "extern", "static", "const", "volatile", "register",
"inline", "restrict", "auto", "void",
"class", "namespace", "template", "typename", "using", "public",
"private", "protected", "virtual", "friend", "this", "new", "delete",
"try", "catch", "throw", "const_cast", "dynamic_cast", "reinterpret_cast",
"static_cast", "typeid", "explicit", "mutable", "operator", "and", "or",
"not", "xor", "noexcept", "nullptr", "override", "final", "constexpr"
```

**Python Keywords:**
```c
"False", "None", "True", "and", "as", "assert", "async", "await",
"break", "class", "continue", "def", "del", "elif", "else", "except",
"finally", "for", "from", "global", "if", "import", "in", "is",
"lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
"while", "with", "yield"
```

### Types

Data type names:

**C/C++ Types:**
```c
"int", "char", "float", "double", "long", "short", "signed",
"unsigned", "bool", "_Bool", "size_t", "ssize_t", "FILE",
"string", "vector", "list", "map", "set", "array", "shared_ptr"
```

**Python Types:**
```c
"int", "float", "str", "bool", "list", "tuple", "dict", "set",
"frozenset", "bytes", "bytearray", "complex", "range", "type"
```

### Constants

Predefined constant values:

**C/C++ Constants:**
```c
"NULL", "true", "false", "TRUE", "FALSE", "EOF", "stdin", "stdout", "stderr"
```

**Python Constants:**
```c
"True", "False", "None", "__name__", "__file__", "__doc__", "__package__"
```

## Color Scheme

ANSI color codes for different token types:

```c
#define COLOR_RESET "\033[0m"
#define COLOR_KEYWORD "\033[38;5;201m"       // Magenta
#define COLOR_TYPE "\033[38;5;38m"         // Blue
#define COLOR_FUNCTION "\033[38;5;226m"    // Yellow
#define COLOR_STRING "\033[38;5;208m"      // Orange
#define COLOR_COMMENT "\033[38;5;242m"     // Gray
#define COLOR_NUMBER "\033[38;5;141m"      // Purple
#define COLOR_OPERATOR "\033[38;5;196m"    // Red
#define COLOR_PREPROC "\033[38;5;129m"     // Purple
#define COLOR_CONSTANT "\033[38;5;85m"     // Cyan
```

## Tokenization Algorithm

The tokenizer scans line by line, identifying tokens by pattern matching:

### Word Matching

```c
static bool matches_word_list(const char* word, int len, 
                               const char* const* list)
{
    for (int i = 0; list[i] != NULL; i++) {
        size_t word_len = strlen(list[i]);
        if (word_len == (size_t)len && 
            strncmp(word, list[i], len) == 0) {
            return true;
        }
    }
    return false;
}
```

### Token Identification

```c
static bool is_keyword(const char* word, int len)
{
    return matches_word_list(word, len, keywords);
}

static bool is_type(const char* word, int len)
{
    return matches_word_list(word, len, types);
}

static bool is_constant(const char* word, int len)
{
    return matches_word_list(word, len, constants);
}
```

### Operator Detection

```c
static bool is_operator(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/' ||
           c == '=' || c == '!' || c == '<' || c == '>' ||
           c == '&' || c == '|' || c == '^' || c == '%';
}

static bool is_two_char_op(const char* s)
{
    return (s[0] == '=' && s[1] == '=') ||
           (s[0] == '!' && s[1] == '=') ||
           (s[0] == '<' && s[1] == '=') ||
           (s[0] == '>' && s[1] == '=') ||
           (s[0] == '+' && s[1] == '+') ||
           (s[0] == '-' && s[1] == '-');
}
```

## Syntax Highlighting Process

### Main Highlighting Function

```c
static void apply_syntax_highlighting(const char* line, int line_len, 
                                       int language)
{
    int i = 0;
    while (i < line_len) {
        // Skip whitespace
        if (isspace(line[i])) {
            i++;
            continue;
        }
        
        // Handle comments
        if (line[i] == '/' && line[i+1] == '/') {
            // Single-line comment
            apply_color(COLOR_COMMENT);
            // ... rest of line
            break;
        }
        
        // Handle preprocessor directives
        if (line[i] == '#') {
            apply_color(COLOR_PREPROC);
            // ... skip to end of directive
            continue;
        }
        
        // Handle strings
        if (line[i] == '"') {
            apply_color(COLOR_STRING);
            // ... parse string literal
            continue;
        }
        
        // Handle numbers
        if (isdigit(line[i])) {
            apply_color(COLOR_NUMBER);
            // ... parse number
            continue;
        }
        
        // Handle words (keywords, types, constants)
        if (isalpha(line[i]) || line[i] == '_') {
            int start = i;
            while (i < line_len && (isalnum(line[i]) || line[i] == '_')) {
                i++;
            }
            int len = i - start;
            
            if (is_keyword(line + start, len)) {
                apply_color(COLOR_KEYWORD);
            } else if (is_type(line + start, len)) {
                apply_color(COLOR_TYPE);
            } else if (is_constant(line + start, len)) {
                apply_color(COLOR_CONSTANT);
            } else {
                // Regular identifier
                reset_color();
            }
            continue;
        }
        
        // Handle operators
        if (is_operator(line[i])) {
            apply_color(COLOR_OPERATOR);
            i++;
            continue;
        }
        
        i++;
    }
}
```

## Language Detection

Language is detected based on file extension:

```c
int syntax_get_language(const char* filepath)
{
    const char* ext = strrchr(filepath, '.');
    if (!ext) return 0;  // Unknown
    
    if (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0 ||
        strcmp(ext, ".cpp") == 0 || strcmp(ext, ".hpp") == 0 ||
        strcmp(ext, ".cc") == 0 || strcmp(ext, ".cxx") == 0) {
        return 1;  // C/C++
    }
    
    if (strcmp(ext, ".py") == 0) {
        return 2;  // Python
    }
    
    return 0;  // Unknown
}
```

## Color Application

### Apply Color

```c
static void append_color(char** result, int* result_len, 
                         int* result_cap, const char* color)
{
    size_t color_len = strlen(color);
    if (*result_len + color_len >= *result_cap) {
        // Grow buffer
        *result_cap *= 2;
        *result = realloc(*result, *result_cap);
    }
    memcpy(*result + *result_len, color, color_len);
    *result_len += color_len;
}
```

### Apply Text

```c
static void append_text(char** result, int* result_len, 
                        int* result_cap, const char* text, 
                        int text_len)
{
    if (*result_len + text_len >= *result_cap) {
        *result_cap *= 2;
        *result = realloc(*result, *result_cap);
    }
    memcpy(*result + *result_len, text, text_len);
    *result_len += text_len;
}
```

## Usage Example

```c
// In draw.c
int language = syntax_get_language(filepath);

for each line in file {
    char* highlighted = syntax_highlight(line, line_len, language);
    render_line(highlighted);
    free(highlighted);
}
```

## Performance Considerations

- **Line-by-line processing** - Each line is processed independently
- **String allocation** - Highlighted strings are allocated per line
- **Pattern matching** - Linear search through keyword/type lists
- **Color overhead** - ANSI escape sequences add ~10-20 bytes per token

Optimization opportunities:
- Cache keyword/type lists in hash table
- Reuse color buffers
- Process only visible lines

## Adding New Language Support

To add support for a new language:

1. **Add keyword list:**
```c
static const char* rust_keywords[] = {
    "fn", "let", "mut", "pub", "use", "mod", "struct", "enum",
    "impl", "trait", "type", "where", "for", "while", "loop",
    "if", "else", "match", "break", "continue", "return",
    "unsafe", "async", "await", "move", "static", "const", NULL
};
```

2. **Add type list:**
```c
static const char* rust_types[] = {
    "i8", "i16", "i32", "i64", "i128", "isize",
    "u8", "u16", "u32", "u64", "u128", "usize",
    "f32", "f64", "bool", "char", "str", "String", NULL
};
```

3. **Add language detection:**
```c
if (strcmp(ext, ".rs") == 0) {
    return 3;  // Rust
}
```

4. **Add highlighting case:**
```c
case 3:  // Rust
    // Use rust_keywords and rust_types
    break;
```

## Limitations

- **No full parser** - Simple pattern matching, not grammatical
- **No nested structures** - Doesn't handle nested comments/strings
- **Limited languages** - Only C/C++ and Python supported
- **No semantic analysis** - Doesn't distinguish variable names from types

## Testing Syntax Highlighting

```bash
# Test C highlighting
./bin/zex test.c

# Test Python highlighting
./bin/zex test.py

# Verify colors display correctly in terminal
# Should see:
# - Keywords in magenta
# - Types in blue
# - Strings in orange
# - Comments in gray
# - Numbers in purple
```

## Best Practices

1. **Keep lists alphabetically sorted** - Easier to maintain
2. **Use consistent naming** - Follow language conventions
3. **Test with real code** - Verify highlighting on actual files
4. **Consider performance** - Avoid expensive operations in hot path
5. **Handle edge cases** - Empty lines, unusual characters