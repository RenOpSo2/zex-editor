#include "config.h"
#include "nodes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_KEY_LEN 63
#define MAX_VAL_LEN 255
#define MAX_JSON_SIZE 1000000

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

static ConfigEntry config_entries[MAX_CONFIG_ENTRIES];
static int config_entry_count = 0;

static char active_config_path[512] = "";
static time_t last_config_mtime = 0;
static int config_loading = 0;

// Fungsi helper untuk safe string copy
static size_t safe_str_copy(char* dest, size_t dest_size, const char* src, size_t src_len) {
    if (!dest || dest_size == 0) return 0;
    
    size_t copy_len = (src_len < dest_size - 1) ? src_len : dest_size - 1;
    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';
    return copy_len;
}

// Default config values setup
void config_init(void)
{
    config_entry_count = 0;

    // Set default tabsize
    safe_str_copy(config_entries[0].key, sizeof(config_entries[0].key), "tabsize", strlen("tabsize"));
    config_entries[0].type = CONFIG_TYPE_NUMBER;
    config_entries[0].number_val = 4.0;

    // Set default mouse
    safe_str_copy(config_entries[1].key, sizeof(config_entries[1].key), "mouse", strlen("mouse"));
    config_entries[1].type = CONFIG_TYPE_BOOL;
    config_entries[1].bool_val = 1; // true

    // Set default show_line_numbers
    safe_str_copy(config_entries[2].key, sizeof(config_entries[2].key), "show_line_numbers", strlen("show_line_numbers"));
    config_entries[2].type = CONFIG_TYPE_BOOL;
    config_entries[2].bool_val = 1; // true

    // Set default auto_indent
    safe_str_copy(config_entries[3].key, sizeof(config_entries[3].key), "auto_indent", strlen("auto_indent"));
    config_entries[3].type = CONFIG_TYPE_BOOL;
    config_entries[3].bool_val = 1; // true

    config_entry_count = 4;
}

// Find entry index by key
static int find_entry(const char* key)
{
    for (int i = 0; i < config_entry_count; i++) {
        if (strcmp(config_entries[i].key, key) == 0) {
            return i;
        }
    }
    return -1;
}

// Simple JSON parser
// Improved parse function
static void parse_json(const char* json_str, size_t json_len) {
    // 1. VALIDASI INPUT
    if (!json_str || json_len == 0 || json_len > MAX_JSON_SIZE) {
        // Log error
        return;
    }
    
    const char* p = json_str;
    const char* end = json_str + json_len;
    
    // 2. SKIP WS dengan batas
    while (p < end && isspace(*p)) p++;
    if (p >= end || *p != '{') return;
    p++; // skip '{'
    
    while (p < end && *p && *p != '}') {
        // Skip whitespace
        while (p < end && isspace(*p)) p++;
        if (p >= end || *p == '}') break;
        
        // 3. VALIDASI KEY
        if (*p != '"') {
            p++;
            continue;
        }
        p++; // skip quote
        
        // 4. PARSE KEY DENGAN BOUNDARY CHECK
        char key[MAX_KEY_LEN + 1] = {0};  // +1 untuk null terminator
        int key_len = 0;
        
        while (p < end && *p && *p != '"' && key_len < MAX_KEY_LEN) {
            key[key_len++] = *p++;
        }
        key[key_len] = '\0';
        
        // 5. VALIDASI KEY TERMINASI
        if (p >= end || *p != '"') {
            // Key tidak valid, skip
            while (p < end && *p && *p != ',') p++;
            continue;
        }
        p++; // skip closing quote
        
        // Skip whitespace
        while (p < end && isspace(*p)) p++;
        if (p >= end || *p != ':') continue;
        p++; // skip ':'
        while (p < end && isspace(*p)) p++;
        
        // 6. PARSE VALUE DENGAN BOUNDARY CHECK
        if (p < end && *p == '"') {
            p++; // skip quote
            char val_str[MAX_VAL_LEN + 1] = {0};
            int val_len = 0;
            
            while (p < end && *p && *p != '"' && val_len < MAX_VAL_LEN) {
                // 7. HANDLE ESCAPED CHARACTERS
                if (*p == '\\' && (p + 1) < end) {
                    switch (*(p + 1)) {
                        case '"':  val_str[val_len++] = '"';  p += 2; break;
                        case '\\': val_str[val_len++] = '\\'; p += 2; break;
                        case 'n':  val_str[val_len++] = '\n'; p += 2; break;
                        case 't':  val_str[val_len++] = '\t'; p += 2; break;
                        default:   val_str[val_len++] = *p++; break;
                    }
                    continue;
                }
                val_str[val_len++] = *p++;
            }
            val_str[val_len] = '\0';
            
            if (p < end && *p == '"') p++;
            
            config_set_string(key, val_str);
            
        } else if (p + 4 <= end && strncmp(p, "true", 4) == 0) {
            config_set_bool(key, 1);
            p += 4;
            
        } else if (p + 5 <= end && strncmp(p, "false", 5) == 0) {
            config_set_bool(key, 0);
            p += 5;
            
        } else if (p < end && (isdigit(*p) || *p == '-' || *p == '.')) {
            // 8. PARSE NUMBER DENGAN BOUNDARY
            char num_str[64] = {0};
            int num_len = 0;
            
            while (p < end && num_len < 63 && 
                   (isdigit(*p) || *p == '.' || *p == '-' || 
                    *p == 'e' || *p == 'E' || *p == '+')) {
                num_str[num_len++] = *p++;
            }
            num_str[num_len] = '\0';
            
            // 9. SAFE NUMBER PARSING
            char* endptr;
            double val = strtod(num_str, &endptr);
            if (endptr != num_str) {  // Valid number
                config_set_number(key, val);
            }
        } else {
            // Unknown token, skip safely
            while (p < end && *p && *p != ',' && *p != '}') p++;
        }
        
        // 10. SKIP COMMA DENGAN BOUNDARY
        while (p < end && isspace(*p)) p++;
        if (p < end && *p == ',') {
            p++; // skip comma
        }
    }
}

// Write/update config file
static void write_config_file(void)
{
    if (config_loading) return;
    if (active_config_path[0] == '\0') {
        // Default to ./zex.json if none active
        safe_str_copy(active_config_path, sizeof(active_config_path), "./zex.json", strlen("./zex.json"));
    }

    FILE* f = fopen(active_config_path, "w");
    if (!f) return;

    fprintf(f, "{\n");
    for (int i = 0; i < config_entry_count; i++) {
        fprintf(f, "  \"%s\": ", config_entries[i].key);
        if (config_entries[i].type == CONFIG_TYPE_NUMBER) {
            fprintf(f, "%g", config_entries[i].number_val);
        } else if (config_entries[i].type == CONFIG_TYPE_BOOL) {
            fprintf(f, "%s", config_entries[i].bool_val ? "true" : "false");
        } else if (config_entries[i].type == CONFIG_TYPE_STRING) {
            fprintf(f, "\"%s\"", config_entries[i].string_val);
        }
        if (i < config_entry_count - 1) {
            fprintf(f, ",\n");
        } else {
            fprintf(f, "\n");
        }
    }
    fprintf(f, "}\n");
    fclose(f);

    // Update mtime so we don't trigger self-reload
    struct stat st;
    if (stat(active_config_path, &st) == 0) {
        last_config_mtime = st.st_mtime;
    }
}

// Load config helper from file
static int load_config_file(const char* path)
{
    FILE* f = fopen(path, "r");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buf = malloc(size + 1);
    if (!buf) {
        fclose(f);
        return 0;
    }

    size_t read_bytes = fread(buf, 1, size, f);
    buf[read_bytes] = '\0';
    fclose(f);

    config_loading = 1;
    parse_json(buf, strlen(buf));
    config_loading = 0;
    free(buf);

    safe_str_copy(active_config_path, sizeof(active_config_path), path, strlen(path));
    struct stat st;
    if (stat(path, &st) == 0) {
        last_config_mtime = st.st_mtime;
    }
    return 1;
}

// Validate schemas
int config_validate(const char* key, const char* raw_val, SchemaError* err_out)
{
    if (strcmp(key, "tabsize") == 0) {
        // must be a positive integer
        for (int i = 0; raw_val[i] != '\0'; i++) {
            if (!isdigit((unsigned char)raw_val[i])) {
                if (err_out) {
                    safe_str_copy(err_out->key, sizeof(err_out->key), key, strlen(key));
                    safe_str_copy(err_out->error_msg, sizeof(err_out->error_msg), "tabsize must be a positive number", strlen("tabsize must be a positive number"));
                }
                return 0;
            }
        }
        int val = atoi(raw_val);
        if (val <= 0 || val > 16) {
            if (err_out) {
                safe_str_copy(err_out->key, sizeof(err_out->key), key, strlen(key));
                safe_str_copy(err_out->error_msg, sizeof(err_out->error_msg), "tabsize must be between 1 and 16", strlen("tabsize must be between 1 and 16"));
            }
            return 0;
        }
    } else if (strcmp(key, "mouse") == 0 || strcmp(key, "show_line_numbers") == 0 || strcmp(key, "auto_indent") == 0) {
        if (strcmp(raw_val, "true") != 0 && strcmp(raw_val, "false") != 0 &&
                strcmp(raw_val, "1") != 0 && strcmp(raw_val, "0") != 0) {
            if (err_out) {
                safe_str_copy(err_out->key, sizeof(err_out->key), key, strlen(key));
                safe_str_copy(err_out->error_msg, sizeof(err_out->error_msg), "value must be boolean (true/false or 1/0)", strlen("value must be boolean (true/false or 1/0)"));
            }
            return 0;
        }
    }
    return 1;
}

void config_load(int argc, char* argv[])
{
    config_loading = 1;
    // 1. Initialise defaults (already done, but call config_init just in case)
    config_init();

    // 2. Load ~/.zexrc or ./zex.json
    int loaded = 0;
    // Check local ./zex.json first
    if (access("./zex.json", F_OK) == 0) {
        loaded = load_config_file("./zex.json");
    }

    // Check ~/.zexrc if not loaded
    if (!loaded) {
        const char* home = getenv("HOME");
        if (home) {
            char home_rc[512];
            snprintf(home_rc, sizeof(home_rc), "%s/.zexrc", home);
            if (access(home_rc, F_OK) == 0) {
                loaded = load_config_file(home_rc);
            }
        }
    }

    // 3. Load CLI arguments e.g., --tabsize 4 or --mouse false
    for (int i = 1; i < argc - 1; i++) {
        if (strncmp(argv[i], "--", 2) == 0) {
            const char* key = argv[i] + 2;
            const char* val = argv[i + 1];

            // Validate value
            SchemaError err;
            if (config_validate(key, val, &err)) {
                if (strcmp(key, "tabsize") == 0) {
                    config_set_number(key, atof(val));
                } else if (strcmp(key, "mouse") == 0 || strcmp(key, "show_line_numbers") == 0 || strcmp(key, "auto_indent") == 0) {
                    config_set_bool(key, (strcmp(val, "true") == 0 || strcmp(val, "1") == 0));
                } else {
                    config_set_string(key, val);
                }
            } else {
                fprintf(stderr, "Config Validation Error: key '%s' has invalid value '%s'. %s\n", err.key, val, err.error_msg);
            }
        }
    }
    config_loading = 0;
}

double config_get_number(const char* key, double default_val)
{
    int idx = find_entry(key);
    if (idx != -1 && config_entries[idx].type == CONFIG_TYPE_NUMBER) {
        return config_entries[idx].number_val;
    }
    return default_val;
}

int config_get_bool(const char* key, int default_val)
{
    int idx = find_entry(key);
    if (idx != -1 && config_entries[idx].type == CONFIG_TYPE_BOOL) {
        return config_entries[idx].bool_val;
    }
    return default_val;
}

const char* config_get_string(const char* key, const char* default_val)
{
    int idx = find_entry(key);
    if (idx != -1 && config_entries[idx].type == CONFIG_TYPE_STRING) {
        return config_entries[idx].string_val;
    }
    return default_val;
}

void config_set_number(const char* key, double val)
{
    int idx = find_entry(key);
    if (idx == -1) {
        if (config_entry_count < MAX_CONFIG_ENTRIES) {
            idx = config_entry_count++;
            safe_str_copy(config_entries[idx].key, sizeof(config_entries[idx].key), key, strlen(key));
        } else {
            return;
        }
    }
    config_entries[idx].type = CONFIG_TYPE_NUMBER;
    config_entries[idx].number_val = val;
    write_config_file();
}

void config_set_bool(const char* key, int val)
{
    int idx = find_entry(key);
    if (idx == -1) {
        if (config_entry_count < MAX_CONFIG_ENTRIES) {
            idx = config_entry_count++;
            safe_str_copy(config_entries[idx].key, sizeof(config_entries[idx].key), key, strlen(key));
        } else {
            return;
        }
    }
    config_entries[idx].type = CONFIG_TYPE_BOOL;
    config_entries[idx].bool_val = val;
    write_config_file();
}

void config_set_string(const char* key, const char* val)
{
    int idx = find_entry(key);
    if (idx == -1) {
        if (config_entry_count < MAX_CONFIG_ENTRIES) {
            idx = config_entry_count++;
            safe_str_copy(config_entries[idx].key, sizeof(config_entries[idx].key), key, strlen(key));
        } else {
            return;
        }
    }
    config_entries[idx].type = CONFIG_TYPE_STRING;
    safe_str_copy(config_entries[idx].string_val, sizeof(config_entries[idx].string_val), val, strlen(val));
    write_config_file();
}

void config_set_filepath(const char* path)
{
    safe_str_copy(active_config_path, sizeof(active_config_path), path, strlen(path));
}

void config_watch(struct global* global)
{
    if (active_config_path[0] == '\0') return;

    struct stat st;
    if (stat(active_config_path, &st) == 0) {
        if (st.st_mtime != last_config_mtime) {
            // Reload config
            load_config_file(active_config_path);
            pgb_replace_str(&global->msg, "Configuration reloaded automatically.", &global->arena);
        }
    }
}
