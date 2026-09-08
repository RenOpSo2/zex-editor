# Undo/Redo System

The undo/redo system in Zex tracks editing actions to allow reverting and reapplying changes. It uses a stack-based approach with fixed-size stacks for memory efficiency.

## Action Types

The system supports three types of actions:

```c
enum action_type {
    action_insert,   // Text insertion
    action_delete,   // Text deletion
    action_replace    // Text replacement
};
```

## Action Structure

Each action stores the information needed to reverse it:

```c
struct action {
    enum action_type type;                    // Action type
    char data[MAX_SEARCH_QUERY_LEN];          // Inserted/deleted/replaced text
    char old_data[MAX_SEARCH_QUERY_LEN];      // Original text (for replace)
    uint32_t pos;                             // Cursor position
    uint32_t len;                             // Length of data
    uint32_t old_len;                         // Length of old data (for replace)
};
```

## Stack Management

### Undo Stack

Stores actions that can be undone:

```c
struct action undo_stack[UNDO_STACK_SIZE];  // 100 actions max
uint32_t undo_count;                        // Current number of actions
```

### Redo Stack

Stores undone actions that can be redone:

```c
struct action redo_stack[UNDO_STACK_SIZE];  // 100 actions max
uint32_t redo_count;                        // Current number of actions
```

### Stack Limits

- **Maximum stack size**: 100 actions (UNDO_STACK_SIZE)
- **Maximum action data**: 4096 bytes (MAX_SEARCH_QUERY_LEN)
- **Fixed-size stacks**: Prevents unbounded memory growth

## Saving Actions

### Save Insert Action

```c
void undo_save_insert(struct global* global, char ch, uint32_t pos)
{
    undo_save_action(global, action_insert, &ch, 1, pos);
}
```

Usage:
```c
// Before inserting character
uint32_t pos = pgb_cursor_pos(&global->text);
undo_save_insert(global, 'x', pos);
pgb_insert(&global->text, 'x', &global->arena);
```

### Save Delete Action

```c
void undo_save_delete(struct global* global, char ch, uint32_t pos)
{
    undo_save_action(global, action_delete, &ch, 1, pos);
}
```

Usage:
```c
// Before deleting character
uint32_t pos = pgb_cursor_pos(&global->text);
char ch = get_char_at_cursor(&global->text);
undo_save_delete(global, ch, pos);
pgb_delete(&global->text);
```

### Save Replace Action

```c
void undo_save_replace(struct global* global, const char* new_text, 
                       uint32_t new_len, const char* old_text, 
                       uint32_t old_len, uint32_t pos)
{
    if (!global || !new_text || !old_text) return;
    if (global->undo_count >= UNDO_STACK_SIZE) return;

    struct action* act = &global->undo_stack[global->undo_count];
    act->type = action_replace;
    act->pos = pos;
    act->len = new_len;
    act->old_len = old_len;

    // Copy new text
    uint32_t copy_len = new_len < MAX_SEARCH_QUERY_LEN ? new_len : MAX_SEARCH_QUERY_LEN;
    for (uint32_t i = 0; i < copy_len; i++) {
        act->data[i] = new_text[i];
    }

    // Copy old text
    uint32_t old_copy_len = old_len < MAX_SEARCH_QUERY_LEN ? old_len : MAX_SEARCH_QUERY_LEN;
    for (uint32_t i = 0; i < old_copy_len; i++) {
        act->old_data[i] = old_text[i];
    }

    global->undo_count++;
    global->redo_count = 0;  // Clear redo stack
}
```

Usage:
```c
// Before replacing text
uint32_t pos = pgb_cursor_pos(&global->text);
char old_text[MAX_SEARCH_QUERY_LEN];
uint32_t old_len = get_selection_text(old_text, sizeof(old_text));
undo_save_replace(global, new_text, new_len, old_text, old_len, pos);
pgb_replace_str(&global->text, new_text, &global->arena);
```

### Batch Operations

For efficiency with large text, use batch operations:

```c
void undo_save_batch_insert(struct global* global, const char* text, 
                           uint32_t len, uint32_t pos)
{
    undo_save_action(global, action_insert, text, len, pos);
}

void undo_save_batch_delete(struct global* global, const char* text, 
                           uint32_t len, uint32_t pos)
{
    undo_save_action(global, action_delete, text, len, pos);
}
```

## Performing Undo

```c
void undo_perform(struct global* global)
{
    if (!global || global->undo_count == 0) return;

    struct action* act = &global->undo_stack[global->undo_count - 1];

    // Save to redo stack
    if (global->redo_count < UNDO_STACK_SIZE) {
        global->redo_stack[global->redo_count] = *act;
        global->redo_count++;
    }

    // Perform undo based on action type
    if (act->type == action_insert) {
        // Undo insert = delete the inserted text
        pgb_delete_range(&global->text, act->pos, act->pos + act->len);
    } else if (act->type == action_delete) {
        // Undo delete = insert the deleted text
        pgb_move_to_pos(&global->text, act->pos);
        pgb_insert_str(&global->text, act->data, &global->arena);
    } else if (act->type == action_replace) {
        // Undo replace = restore original text
        pgb_move_to_pos(&global->text, act->pos);
        pgb_delete_range(&global->text, act->pos, act->pos + act->len);
        pgb_move_to_pos(&global->text, act->pos);
        pgb_insert_str(&global->text, act->old_data, &global->arena);
    }

    global->undo_count--;
}
```

## Performing Redo

```c
void redo_perform(struct global* global)
{
    if (!global || global->redo_count == 0) return;

    struct action* act = &global->redo_stack[global->redo_count - 1];

    // Perform redo based on action type
    if (act->type == action_insert) {
        // Redo insert = insert the text back
        pgb_move_to_pos(&global->text, act->pos);
        pgb_insert_str(&global->text, act->data, &global->arena);
    } else if (act->type == action_delete) {
        // Redo delete = delete the text again
        pgb_delete_range(&global->text, act->pos, act->pos + act->len);
    } else if (act->type == action_replace) {
        // Redo replace = apply replacement again
        pgb_move_to_pos(&global->text, act->pos);
        pgb_delete_range(&global->text, act->pos, act->pos + act->old_len);
        pgb_move_to_pos(&global->text, act->pos);
        pgb_insert_str(&global->text, act->data, &global->arena);
    }

    // Move action back to undo stack
    global->redo_count--;
    global->undo_count++;
}
```

## Stack Behavior

### New Action Clears Redo Stack

When a new action is performed, the redo stack is cleared:

```c
static void undo_save_action(...)
{
    // ... save action
    global->redo_count = 0;  // Clear redo stack on new action
}
```

This ensures that redo only contains actions that were undone, not actions that were invalidated by new edits.

### Stack Overflow Protection

If stacks are full, new actions are not saved:

```c
if (global->undo_count >= UNDO_STACK_SIZE) return;
```

## Usage in Input Handler

The undo/redo system is integrated into the input handler:

```c
// In input.c
enum result input_update(struct global* global)
{
    uint32_t ch = term_read(&byte);
    
    // Handle Ctrl+U (undo)
    if (ch == CTRL_KEY('u')) {
        undo_perform(global);
        return ok;
    }
    
    // Handle Ctrl+Y (redo)
    if (ch == CTRL_KEY('y')) {
        redo_perform(global);
        return ok;
    }
    
    // Handle regular character input
    if (is_printable(ch)) {
        uint32_t pos = pgb_cursor_pos(&global->text);
        undo_save_insert(global, ch, pos);
        pgb_insert(&global->text, ch, &global->arena);
        return ok;
    }
    
    // Handle backspace
    if (ch == 127) {  // Backspace
        uint32_t pos = pgb_cursor_pos(&global->text);
        char ch = get_char_before_cursor(&global->text);
        undo_save_delete(global, ch, pos);
        pgb_delete(&global->text);
        return ok;
    }
    
    // ... other input handling
}
```

## Clearing History

To clear all undo/redo history:

```c
void undo_clear_history(struct global* global)
{
    if (!global) return;
    global->undo_count = 0;
    global->redo_count = 0;
}
```

Useful when:
- Opening a new file
- Performing bulk operations that shouldn't be undoable
- Resetting editor state

## Checking Stack Status

```c
bool undo_can_undo(struct global* global)
{
    return global && global->undo_count > 0;
}

bool undo_can_redo(struct global* global)
{
    return global && global->redo_count > 0;
}
```

## Memory Usage

Each action uses:
- **Action struct**: ~8KB (two 4KB buffers + metadata)
- **Stack total**: ~800KB (100 actions × 8KB)

This is acceptable for a text editor and bounded by fixed stack sizes.

## Performance Characteristics

- **Save action**: O(1) - just copy to stack
- **Undo**: O(n) where n is action length (delete/insert)
- **Redo**: O(n) where n is action length (insert/delete)
- **Stack operations**: O(1) - array indexing

## Limitations

1. **Fixed stack size** - Only 100 actions can be undone
2. **Action size limit** - Actions limited to 4KB of data
3. **No grouping** - Each character is a separate action (inefficient for paste)
4. **Position-based** - Uses byte positions, not semantic positions

## Improvements Possible

1. **Action grouping** - Group consecutive similar actions
2. **Semantic undo** - Undo by logical operation (word, line)
3. **Dynamic stack size** - Grow stack based on available memory
4. **Compression** - Compress action data to save space
5. **Undo tree** - Branching undo for non-linear history

## Testing Undo/Redo

```bash
# Run undo tests
make test-undo
make test-enhanced-undo
make test-large-undo

# Manual test
./bin/zex test.c
# Type: Hello World
# Press Ctrl+U repeatedly to undo
# Press Ctrl+Y repeatedly to redo
```

## Best Practices

1. **Save before modifying** - Always save undo state before buffer changes
2. **Use batch operations** - For large text, use batch insert/delete
3. **Clear appropriately** - Clear history on file operations
4. **Check stack limits** - Handle stack overflow gracefully
5. **Test edge cases** - Empty stacks, large actions, etc.