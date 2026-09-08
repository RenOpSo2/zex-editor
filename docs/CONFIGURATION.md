# Configuration System

The Zex configuration system allows customization of editor behavior through JSON config files and command-line arguments.

## Configuration Sources

Configuration is loaded from multiple sources in order of precedence (highest first):

1. **Command-line arguments** (e.g., `--tabsize 8`)
2. **Local config file** (`./zex.json`)
3. **User config file** (`~/.zexrc`)
4. **Default values** (hardcoded in `config_init()`)

## Config File Formats

### JSON Format (zex.json)

The preferred format is JSON:

```json
{
  "tabsize": 4,
  "mouse": true,
  "show_line_numbers": true,
  "auto_indent": true
}
```

### Zexrc Format (~/.zexrc)

Legacy format (simple key=value pairs):

```
tabsize=4
mouse=1
show_line_numbers=1
auto_indent=1
```

## Available Configuration Options

### tabsize
- **Type**: Number
- **Default**: 4
- **Range**: 1-16
- **Description**: Number of spaces per tab character

```c
int tabsize = (int)config_get_number("tabsize", 4);
```

### mouse
- **Type**: Boolean
- **Default**: true
- **Description**: Enable mouse click support

```c
if (config_get_bool("mouse", 1)) {
    // Enable mouse tracking
}
```

### show_line_numbers
- **Type**: Boolean
- **Default**: true
- **Description**: Display line numbers in gutter

```c
if (config_get_bool("show_line_numbers", 1)) {
    draw_line_number(line_num);
}
```

### auto_indent
- **Type**: Boolean
- **Default**: true
- **Description**: Automatically indent new lines

```c
if (config_get_bool("auto_indent", 1)) {
    auto_indent(global);
}
```

## Configuration API

### Initialization

```c
#include "config.h"

// Load config from all sources
config_load(argc, argv);
```

### Getting Values

```c
// Get number with default
double tabsize = config_get_number("tabsize", 4.0);

// Get boolean with default
int mouse_enabled = config_get_bool("mouse", 1);

// Get string with default
const char* theme = config_get_string("theme", "default");
```

### Setting Values

```c
// Set number (writes to active config file)
config_set_number("tabsize", 8.0);

// Set boolean
config_set_bool("mouse", 0);

// Set string
config_set_string("theme", "dark");
```

### Setting Config File Path

```c
// Explicitly set the active config file
config_set_filepath("/path/to/config.json");
```

### Runtime Watching

The editor automatically watches the config file for changes:

```c
// Called in main loop to check for config changes
config_watch(&global);
```

If the config file is modified, it's automatically reloaded.

## JSON Parser Implementation

The configuration system includes a simple JSON parser that handles basic JSON objects with string, number, and boolean values.

### Parser Overview

```c
static void parse_json(const char* json_str, size_t json_len)
{
    // 1. Validate input
    if (!json_str || json_len == 0 || json_len > MAX_JSON_SIZE) {
        return;
    }
    
    // 2. Skip whitespace
    while (p < end && isspace(*p)) p++;
    
    // 3. Expect opening brace
    if (*p != '{') return;
    p++;
    
    // 4. Parse key-value pairs
    while (p < end) {
        // Parse key
        const char* key_start = p;
        // ... extract key string
        
        // Skip to colon
        while (p < end && *p != ':') p++;
        p++;
        
        // Skip whitespace
        while (p < end && isspace(*p)) p++;
        
        // Parse value (string, number, or boolean)
        if (*p == '"') {
            // String value
            // ... extract string
        } else if (isdigit(*p) || *p == '-') {
            // Number value
            // ... parse number
        } else if (strncmp(p, "true", 4) == 0) {
            // Boolean true
            // ... set boolean value
        } else if (strncmp(p, "false", 5) == 0) {
            // Boolean false
            // ... set boolean value
        }
        
        // Skip to next key or closing brace
        while (p < end && *p != ',' && *p != '}') p++;
    }
}
```

### Schema Validation

Configuration values are validated against their expected types:

```c
int config_validate(const char* key, const char* raw_val, SchemaError* err_out)
{
    // Find entry in config table
    int idx = find_entry(key);
    if (idx < 0) {
        if (err_out) {
            snprintf(err_out->error_msg, sizeof(err_out->error_msg),
                     "Unknown config key: %s", key);
        }
        return 0;
    }
    
    // Validate based on type
    switch (config_entries[idx].type) {
        case CONFIG_TYPE_NUMBER:
            // Check if valid number
            if (!is_valid_number(raw_val)) {
                return 0;
            }
            break;
        case CONFIG_TYPE_BOOL:
            // Check if valid boolean
            if (!is_valid_bool(raw_val)) {
                return 0;
            }
            break;
        case CONFIG_TYPE_STRING:
            // Strings are always valid
            break;
    }
    
    return 1;
}
```

## Config File Watching

The editor monitors the config file for changes using modification time:

```c
void config_watch(struct global* global)
{
    if (active_config_path[0] == '\0') return;
    
    struct stat st;
    if (stat(active_config_path, &st) != 0) return;
    
    // Check if file was modified
    if (st.st_mtime != last_config_mtime) {
        last_config_mtime = st.st_mtime;
        
        // Reload config
        config_loading = 1;
        load_config_file(active_config_path);
        config_loading = 0;
        
        // Re-render screen with new settings
        draw_update(global);
    }
}
```

## Default Configuration

Default values are set in `config_init()`:

```c
void config_init(void)
{
    config_entry_count = 0;
    
    // Set default tabsize
    safe_str_copy(config_entries[0].key, sizeof(config_entries[0].key), 
                  "tabsize", strlen("tabsize"));
    config_entries[0].type = CONFIG_TYPE_NUMBER;
    config_entries[0].number_val = 4.0;
    
    // Set default mouse
    safe_str_copy(config_entries[1].key, sizeof(config_entries[1].key), 
                  "mouse", strlen("mouse"));
    config_entries[1].type = CONFIG_TYPE_BOOL;
    config_entries[1].bool_val = 1;
    
    // Set default show_line_numbers
    safe_str_copy(config_entries[2].key, sizeof(config_entries[2].key), 
                  "show_line_numbers", strlen("show_line_numbers"));
    config_entries[2].type = CONFIG_TYPE_BOOL;
    config_entries[2].bool_val = 1;
    
    // Set default auto_indent
    safe_str_copy(config_entries[3].key, sizeof(config_entries[3].key), 
                  "auto_indent", strlen("auto_indent"));
    config_entries[3].type = CONFIG_TYPE_BOOL;
    config_entries[3].bool_val = 1;
    
    config_entry_count = 4;
}
```

## Adding New Configuration Options

To add a new configuration option:

1. **Add to config_init()**:
```c
void config_init(void)
{
    // ... existing configs
    
    // Add new option
    safe_str_copy(config_entries[config_entry_count].key, 
                  sizeof(config_entries[config_entry_count].key),
                  "new_option", strlen("new_option"));
    config_entries[config_entry_count].type = CONFIG_TYPE_NUMBER;
    config_entries[config_entry_count].number_val = 42.0;
    config_entry_count++;
}
```

2. **Use in code**:
```c
double value = config_get_number("new_option", 42.0);
```

3. **Update documentation**:
   - Add to this document
   - Update README.md if user-facing

## Config Storage Limits

- **Maximum entries**: 32 (MAX_CONFIG_ENTRIES)
- **Maximum key length**: 63 characters
- **Maximum value length**: 255 characters
- **Maximum JSON size**: 1MB

## Error Handling

### Invalid JSON

If the JSON file is malformed, the parser silently fails and defaults are used.

### Invalid Values

Values that don't match the expected type are ignored:
```c
// If user sets "tabsize": "hello" (string instead of number)
// The invalid value is ignored and default (4) is used
```

### Missing Config File

If config files don't exist, the editor uses default values.

## Example Usage

### Setting Up Config for a Project

Create `./zex.json` in your project directory:

```json
{
  "tabsize": 2,
  "mouse": false,
  "show_line_numbers": true,
  "auto_indent": true
}
```

### Command-Line Override

Override config from command line:

```bash
./bin/zex file.c --tabsize 8 --mouse false
```

### Programmatic Config Change

Change config at runtime:

```c
// Change tab size during editing
config_set_number("tabsize", 8.0);

// The change is written to the active config file
// and takes effect immediately
```

## Testing Configuration

Test the configuration system:

```bash
# Run config tests
make test-config

# Manual test with custom config
echo '{"tabsize": 8}' > test_config.json
./bin/zex --config test_config.json
```

## Configuration System Internals

### Config Entry Structure

```c
typedef enum {
    CONFIG_TYPE_NUMBER,
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_STRING
} ConfigType;

typedef struct {
    char key[64];
    ConfigType type;
    double number_val;
    int bool_val;
    char string_val[MAX_SEARCH_QUERY_LEN];
} ConfigEntry;
```

### Global Config State

```c
static ConfigEntry config_entries[MAX_CONFIG_ENTRIES];
static int config_entry_count = 0;
static char active_config_path[512] = "";
static time_t last_config_mtime = 0;
static int config_loading = 0;
```

### Config Loading Flow

```
config_load(argc, argv)
  ↓
config_init() - Set defaults
  ↓
load_config_file("~/.zexrc") - Load user config
  ↓
load_config_file("./zex.json") - Load local config
  ↓
parse CLI arguments - Override with command line
  ↓
Set active_config_path for watching
```

## Best Practices

1. **Keep config simple** - Use only necessary options
2. **Document new options** - Update docs when adding configs
3. **Validate ranges** - Check bounds (e.g., tabsize 1-16)
4. **Provide sensible defaults** - Ensure editor works without config
5. **Test config loading** - Use `make test-config` to verify