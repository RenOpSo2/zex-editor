# Search System

The search system in Zex provides text search functionality using the Knuth-Morris-Pratt (KMP) algorithm for efficient pattern matching. It supports forward and backward navigation through matches.

## Search State

The search state is maintained in the global structure:

```c
struct global {
    // ... other fields
    
    // Search state
    bool search_active;                          // Is search active?
    char search_query[MAX_SEARCH_QUERY_LEN];      // Current search query
    uint32_t search_pos;                          // Current match position
    uint32_t search_match_count;                  // Total matches found
    uint32_t search_query_len;                    // Query length
};
```

## Search Algorithm

### Knuth-Morris-Pratt (KMP)

The search uses the KMP algorithm for O(n) time complexity:

**Advantages:**
- Linear time complexity
- No backtracking in the text
- Efficient for repeated searches
- Works well with paged buffer structure

**Prefix Function:**
```c
// Build prefix function (failure function)
uint32_t pi[MAX_SEARCH_QUERY_LEN];
for (uint32_t i = 1; i < qlen; i++) {
    while (j && q[i] != q[j]) j = pi[j - 1];
    if (q[i] == q[j]) j++;
    pi[i] = j;
}
```

### Search Scan Function

```c
static uint32_t search_scan(const struct paged_gap_buffer* pgb, 
                            const char* q, uint32_t qlen,
                            uint32_t start, uint32_t stop,
                            bool want_last, uint32_t* count)
{
    // Calculate total buffer size
    uint32_t total_size = 0;
    for (struct page* p = pgb->head; p; p = p->next) {
        uint32_t gap_size = p->gap_end - p->gap_start;
        uint32_t page_content = PAGE_CAPACITY - gap_size;
        total_size += page_content;
    }
    
    // Validate range
    if (start >= stop || stop > total_size) return (uint32_t)-1;
    
    // Build KMP prefix function
    uint32_t pi[MAX_SEARCH_QUERY_LEN], j = 0, pos = 0, found = (uint32_t)-1;
    for (uint32_t i = 1; i < qlen; i++) {
        while (j && q[i] != q[j]) j = pi[j - 1];
        if (q[i] == q[j]) j++;
        pi[i] = j;
    }
    
    // Scan each page
    for (struct page* p = pgb->head; p; p = p->next) {
        // Skip pages before start position
        if (pos + page_size <= start) {
            pos += page_size;
            continue;
        }
        
        // Stop if past stop position
        if (pos >= stop) break;
        
        // Scan before-gap and after-gap sections
        uint32_t parts[2] = {p->gap_start, PAGE_CAPACITY};
        uint32_t begins[2] = {0, p->gap_end};
        for (int part = 0; part < 2; part++) {
            for (uint32_t k = begins[part]; k < parts[part]; k++) {
                if (pos >= stop) break;
                
                unsigned char c = (unsigned char)p->data[k];
                while (j && c != (unsigned char)q[j]) j = pi[j - 1];
                if (c == (unsigned char)q[j]) j++;
                
                if (j == qlen) {
                    // Match found
                    uint32_t at = pos + 1 - qlen;
                    if (at >= start && at < stop) {
                        if (count) (*count)++;
                        if (found == (uint32_t)-1 || want_last) found = at;
                    }
                    j = pi[j - 1];
                }
                pos++;
            }
        }
    }
    
    return found;
}
```

## Search Operations

### Initialize Search

```c
void search_init(struct global* global)
{
    if (!global) return;
    global->search_active = false;
    global->search_query[0] = '\0';
    global->search_pos = 0;
    global->search_match_count = 0;
    global->search_query_len = 0;
}
```

### Find First Match

```c
void search_find(struct global* global, const char* query)
{
    if (!global || !query || query[0] == '\0') {
        global->search_active = false;
        return;
    }
    
    // Store query
    strncpy(global->search_query, query, sizeof(global->search_query) - 1);
    global->search_query[sizeof(global->search_query) - 1] = '\0';
    global->search_query_len = (uint32_t)strlen(global->search_query);
    
    // Find first match
    uint32_t first_match = search_scan(&global->text, global->search_query,
                                       global->search_query_len, 0, UINT32_MAX,
                                       false, &global->search_match_count);
    
    global->search_active = (global->search_match_count > 0);
    global->search_pos = first_match;
    
    // Move cursor to match
    if (first_match != (uint32_t)-1) {
        pgb_move_to_pos(&global->text, first_match);
    }
}
```

### Next Match

```c
void search_next(struct global* global)
{
    if (!global || !global->search_active) return;
    
    char* query = global->search_query;
    uint32_t query_len = global->search_query_len;
    
    // Start search after current match
    uint32_t start_pos = (global->search_pos == (uint32_t)-1) ? 
                         0 : global->search_pos + query_len;
    
    // Search forward
    uint32_t next_pos = search_scan(&global->text, query, query_len, 
                                   start_pos, UINT32_MAX, false, NULL);
    
    // Wrap around if not found
    if (next_pos == (uint32_t)-1) {
        next_pos = search_scan(&global->text, query, query_len, 
                              0, start_pos, false, NULL);
    }
    
    // Move to match
    if (next_pos != (uint32_t)-1) {
        global->search_pos = next_pos;
        pgb_move_to_pos(&global->text, next_pos);
    }
}
```

### Previous Match

```c
void search_prev(struct global* global)
{
    if (!global || !global->search_active) return;
    
    char* query = global->search_query;
    
    // Search backward (use want_last=true)
    uint32_t prev_pos = search_scan(&global->text, query, 
                                   global->search_query_len, 
                                   0, global->search_pos, true, NULL);
    
    // Wrap around if not found
    if (prev_pos == (uint32_t)-1) {
        prev_pos = search_scan(&global->text, query, 
                              global->search_query_len, 
                              global->search_pos, UINT32_MAX, true, NULL);
    }
    
    // Move to match
    if (prev_pos != (uint32_t)-1) {
        global->search_pos = prev_pos;
        pgb_move_to_pos(&global->text, prev_pos);
    }
}
```

## Usage in Input Handler

Search is triggered by keybindings:

```c
// In input.c
enum result input_update(struct global* global)
{
    uint32_t ch = term_read(&byte);
    
    // Handle Ctrl+F (start search)
    if (ch == CTRL_KEY('f')) {
        search_mode_active = true;
        return ok;
    }
    
    // Handle Ctrl+N (next match)
    if (ch == CTRL_KEY('n')) {
        search_next(global);
        return ok;
    }
    
    // Handle Ctrl+P (previous match)
    if (ch == CTRL_KEY('p')) {
        search_prev(global);
        return ok;
    }
    
    // Handle ESC (cancel search)
    if (ch == 27) {  // ESC
        search_mode_active = false;
        search_init(global);
        return ok;
    }
    
    // Handle search input
    if (search_mode_active) {
        if (is_printable(ch)) {
            search_input[search_input_len++] = (char)ch;
            search_input[search_input_len] = '\0';
            search_find(global, search_input);
        }
        return ok;
    }
    
    // ... regular input handling
}
```

## Performance Characteristics

- **Search time**: O(n + m) where n is text length, m is query length
- **Memory**: O(m) for prefix function
- **Paged buffer**: Efficient scanning without flattening
- **Wrap-around**: Two scans for wrap (forward + backward)

## Search Limits

- **Maximum query length**: 4096 characters (MAX_SEARCH_QUERY_LEN)
- **Maximum matches**: No explicit limit (counted on scan)
- **Case sensitivity**: Currently case-sensitive
- **Regular expressions**: Not supported (literal match only)

## Defensive Programming

The search implementation includes several safety checks:

### Overflow Protection

```c
// Prevent overflow in size calculation
if (total_size > UINT32_MAX - page_content) {
    total_size = UINT32_MAX;
    break;
}
```

### Gap Validation

```c
// Ensure gap_end >= gap_start to prevent underflow
uint32_t gap_size = (p->gap_end >= p->gap_start) ? 
                   (p->gap_end - p->gap_start) : 0;
```

### Range Validation

```c
// Validate search range
if (start >= stop || stop > total_size) return (uint32_t)-1;
```

## Search Scenarios

### Basic Search

```c
// User presses Ctrl+F
search_mode_active = true;

// User types "hello"
search_find(global, "hello");
// Cursor moves to first "hello"

// User presses Ctrl+N
search_next(global);
// Cursor moves to next "hello"
```

### No Matches

```c
search_find(global, "nonexistent");
// search_active = false
// search_match_count = 0
// Cursor doesn't move
```

### Wrap-Around

```c
// At last match
search_next(global);
// Wraps to first match

// At first match
search_prev(global);
// Wraps to last match
```

## Testing Search

```bash
# Run search tests
make test-search

# Manual test
./bin/zex test.c
# Press Ctrl+F
# Type "function"
# Press Ctrl+N to navigate matches
# Press Ctrl+P to navigate backward
# Press ESC to cancel
```

## Limitations

1. **Case-sensitive** - Doesn't support case-insensitive search
2. **No regex** - Only literal string matching
3. **No whole word** - Matches substrings
4. **No highlight** - Matches not visually highlighted
5. **Single query** - Only one active search at a time

## Possible Improvements

1. **Case-insensitive search** - Add flag for case insensitivity
2. **Regular expressions** - Support regex patterns
3. **Visual highlighting** - Highlight matches in rendered output
4. **Search history** - Remember previous searches
5. **Incremental search** - Search as you type
6. **Whole word option** - Match complete words only
7. **Multiple queries** - Support multiple active searches

## Best Practices

1. **Validate input** - Check for empty/invalid queries
2. **Handle overflow** - Protect against integer overflow
3. **Clear state** - Reset search state on cancel/file change
4. **Update cursor** - Always move cursor to match position
5. **Test edge cases** - Empty buffer, no matches, etc.