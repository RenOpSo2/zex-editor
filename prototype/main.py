import re
import sys
import ctypes

# Warna ANSI untuk tema dark GitHub-style
class Colors:
    RESET = '\033[0m'
    BOLD = '\033[1m'
    
    # Warna GitHub Dark
    KEYWORD = '\033[38;5;204m'      # Pink
    TYPE = '\033[38;5;111m'         # Biru
    FUNCTION = '\033[38;5;220m'     # Kuning
    STRING = '\033[38;5;71m'        # Hijau
    COMMENT = '\033[38;5;244m'      # Abu-abu
    NUMBER = '\033[38;5;215m'       # Orange
    OPERATOR = '\033[38;5;204m'     # Pink
    PREPROC = '\033[38;5;152m'      # Biru muda
    CONSTANT = '\033[38;5;180m'     # Coklat
    PUNCTUATION = '\033[38;5;255m'  # Putih
    VARIABLE = '\033[38;5;140m'     # Ungu

# Regex patterns untuk C syntax highlighting
class CPatterns:
    # Preprocessor directives
    PREPROC = r'#\s*\w+.*'
    
    # Multi-line comments
    MULTILINE_COMMENT = r'/\*.*?\*/'
    
    # Single-line comments
    SINGLELINE_COMMENT = r'//.*'
    
    # String literals (double quote)
    STRING_DOUBLE = r'"(?:\\.|[^"\\])*"'
    
    # Character literals (single quote)
    STRING_SINGLE = r"'(?:\\.|[^'\\])*'"
    
    # Numbers (decimal, hex, octal, binary)
    NUMBER = r'\b(?:0[xX][0-9a-fA-F]+|\b\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?[fFlLuU]?\b'
    
    # Keywords
    KEYWORDS = r'\b(break|case|continue|default|do|else|for|goto|if|return|switch|while|sizeof|typedef|struct|union|enum|extern|static|const|volatile|register|inline|restrict|auto|void)\b'
    
    # Types
    TYPES = r'\b(int|char|float|double|long|short|signed|unsigned|bool|_Bool|complex|_Complex|imaginary|_Imaginary|uint8_t|uint16_t|uint32_t|uint64_t|int8_t|int16_t|int32_t|int64_t|size_t|ssize_t|FILE)\b'
    
    # Constants
    CONSTANTS = r'\b(NULL|true|false|TRUE|FALSE|EOF|stdin|stdout|stderr)\b'
    
    # Function calls (word followed by opening parenthesis)
    FUNCTION = r'\b([a-zA-Z_]\w*)\s*(?=\()'
    
    # Operators
    OPERATORS = r'(\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%=&|^~<>!])'

def highlight_c_regex(code):
    """
    Highlight C code using regex patterns with proper ordering.
    Uses a token-based approach to avoid re-matching ANSI codes.
    """
    result = []
    i = 0
    n = len(code)
    
    while i < n:
        # Skip ANSI escape sequences
        if code[i] == '\033':
            end = code.find('m', i)
            if end != -1:
                result.append(code[i:end+1])
                i = end + 1
                continue
        
        # Try to match patterns at current position
        matched = False
        
        # Check for multi-line comment
        if i + 1 < n and code[i:i+2] == '/*':
            end = code.find('*/', i)
            if end != -1:
                result.append(f"{Colors.COMMENT}{code[i:end+2]}{Colors.RESET}")
                i = end + 2
                matched = True
                continue
        
        # Check for single-line comment
        if i + 1 < n and code[i:i+2] == '//':
            end = code.find('\n', i)
            if end == -1:
                end = n
            result.append(f"{Colors.COMMENT}{code[i:end]}{Colors.RESET}")
            i = end
            matched = True
            continue
        
        # Check for preprocessor
        if code[i] == '#':
            end = i + 1
            while end < n and code[end] != '\n':
                end += 1
            result.append(f"{Colors.PREPROC}{code[i:end]}{Colors.RESET}")
            i = end
            matched = True
            continue
        
        # Check for string literals
        if code[i] == '"':
            end = i + 1
            while end < n and code[end] != '"':
                if code[end] == '\\' and end + 1 < n:
                    end += 2
                else:
                    end += 1
            if end < n:
                result.append(f"{Colors.STRING}{code[i:end+1]}{Colors.RESET}")
                i = end + 1
                matched = True
                continue
        
        # Check for character literals
        if code[i] == "'":
            end = i + 1
            while end < n and code[end] != "'":
                if code[end] == '\\' and end + 1 < n:
                    end += 2
                else:
                    end += 1
            if end < n:
                result.append(f"{Colors.STRING}{code[i:end+1]}{Colors.RESET}")
                i = end + 1
                matched = True
                continue
        
        # Check for identifiers (keywords, types, functions, variables)
        if code[i].isalpha() or code[i] == '_':
            start = i
            while i < n and (code[i].isalnum() or code[i] == '_'):
                i += 1
            word = code[start:i]
            
            # Check if it's a keyword
            if re.match(CPatterns.KEYWORDS, word):
                result.append(f"{Colors.KEYWORD}{word}{Colors.RESET}")
            # Check if it's a type
            elif re.match(CPatterns.TYPES, word):
                result.append(f"{Colors.TYPE}{word}{Colors.RESET}")
            # Check if it's a constant
            elif re.match(CPatterns.CONSTANTS, word):
                result.append(f"{Colors.CONSTANT}{word}{Colors.RESET}")
            # Check if it's a function call
            else:
                j = i
                while j < n and code[j].isspace():
                    j += 1
                if j < n and code[j] == '(':
                    result.append(f"{Colors.FUNCTION}{word}{Colors.RESET}")
                else:
                    result.append(word)
            matched = True
            continue
        
        # Check for numbers
        if code[i].isdigit() or (code[i] == '.' and i + 1 < n and code[i+1].isdigit()):
            start = i
            if code[i] == '0' and i + 1 < n and code[i+1].lower() == 'x':
                i += 2
                while i < n and (code[i].isdigit() or code[i].lower() in 'abcdef'):
                    i += 1
            else:
                while i < n and (code[i].isdigit() or code[i] == '.'):
                    i += 1
                if i < n and code[i].lower() in 'e':
                    i += 1
                    if i < n and code[i] in '+-':
                        i += 1
                    while i < n and code[i].isdigit():
                        i += 1
                if i < n and code[i].lower() in 'flu':
                    i += 1
            result.append(f"{Colors.NUMBER}{code[start:i]}{Colors.RESET}")
            matched = True
            continue
        
        # Check for operators
        if code[i] in '+-*/%=&|^~<>!':
            op = code[i]
            if i + 1 < n:
                two_char = code[i:i+2]
                if two_char in ('++', '--', '<<', '>>', '<=', '>=', '==', '!=', '&&', '||', '+=', '-=', '*=', '/=', '%=', '&=', '|=', '^=', '->'):
                    op = two_char
                    i += 1
            result.append(f"{Colors.OPERATOR}{op}{Colors.RESET}")
            i += 1
            matched = True
            continue
        
        # Default: append character as-is
        result.append(code[i])
        i += 1
    
    return ''.join(result)

def highlight_c_github(code):
    """
    Legacy function maintained for compatibility.
    Uses the new regex-based implementation.
    """
    return highlight_c_regex(code)

def print_dark_theme(code):
    # Tambah background gelap
    print('\033[40m', end='')
    print(highlight_c_github(code))
    print(Colors.RESET, end='')

# Python-C Interop Functions
# These functions are designed to be called from C code using Python C API

def highlight_line(line):
    """
    Highlight a single line of C code.
    Returns a string with ANSI color codes.
    """
    return highlight_c_regex(line)

def highlight_chunk(code_chunk):
    """
    Highlight a chunk of C code (multiple lines).
    Returns a string with ANSI color codes.
    """
    return highlight_c_regex(code_chunk)

# C-compatible function using ctypes
def highlight_c_buffer(input_buffer, input_len, output_buffer, output_len):
    """
    C-compatible function for syntax highlighting.
    
    Args:
        input_buffer: ctypes.c_char_p - input C string
        input_len: ctypes.c_int - length of input
        output_buffer: ctypes.c_char_p - output buffer
        output_len: ctypes.c_int - maximum output length
    
    Returns:
        Number of bytes written to output buffer
    """
    try:
        # Convert C string to Python string
        code = input_buffer[:input_len].decode('utf-8')
        
        # Highlight the code
        highlighted = highlight_c_regex(code)
        
        # Convert back to C string
        highlighted_bytes = highlighted.encode('utf-8')
        
        # Copy to output buffer if space allows
        copy_len = min(len(highlighted_bytes), output_len - 1)
        ctypes.memmove(output_buffer, highlighted_bytes, copy_len)
        output_buffer[copy_len] = b'\0'  # Null-terminate
        
        return copy_len
    except Exception as e:
        return 0

# Export function for ctypes
HIGHLIGHT_FUNC = ctypes.CFUNCTYPE(
    ctypes.c_int,  # return type
    ctypes.c_char_p,  # input_buffer
    ctypes.c_int,     # input_len
    ctypes.c_char_p,  # output_buffer
    ctypes.c_int      # output_len
)

# Create the callable object
highlight_c_buffer_c = HIGHLIGHT_FUNC(highlight_c_buffer)

# Contoh kode C
sample_code = """#include <stdio.h>
#include <stdlib.h>

#define MAX 100
#define PI 3.14159

/* Program sederhana */
int main(int argc, char *argv[]) {
    // Deklarasi variabel
    int angka = 42;
    char huruf = 'A';
    float phi = 3.14;
    double hasil = 0.0;
    
    const char *pesan = "Hello World!";
    
    printf("Angka: %d\\n", angka);
    printf("Huruf: %c\\n", huruf);
    
    if (angka > 0) {
        printf("Positif!\\n");
    } else if (angka < 0) {
        printf("Negatif!\\n");
    } else {
        printf("Nol!\\n");
    }
    
    for (int i = 0; i < 10; i++) {
        hasil += i * 2.5;
    }
    
    while (angka > 0) {
        angka--;
    }
    
    return 0;
}"""

if __name__ == "__main__":
    print("\n" + "="*60)
    print("GITHUB DARK THEME - C SYNTAX HIGHLIGHT")
    print("="*60 + "\n")
    
    print_dark_theme(sample_code)
    
    # Baca dari file jika ada argumen
    if len(sys.argv) > 1:
        try:
            with open(sys.argv[1], 'r') as f:
                code = f.read()
                print("\n" + "="*60)
                print(f"FILE: {sys.argv[1]}")
                print("="*60 + "\n")
                print_dark_theme(code)
        except FileNotFoundError:
            print(f"❌ File {sys.argv[1]} tidak ditemukan!")
