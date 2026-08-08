#ifndef CONFIG_H
#define CONFIG_H

#include "global.h"

// Schema errors
typedef struct {
    char key[64];
    char error_msg[128];
} SchemaError;

// Initialize config defaults
void config_init(void);

// Load configs from ~/.zexrc, ./zex.json, and CLI args
void config_load(int argc, char* argv[]);

// Getters
double config_get_number(const char* key, double default_val);
int config_get_bool(const char* key, int default_val);
const char* config_get_string(const char* key, const char* default_val);

// Setters (writes/updates to the active file)
void config_set_number(const char* key, double val);
void config_set_bool(const char* key, int val);
void config_set_string(const char* key, const char* val);

// Set the active config file path explicitly
void config_set_filepath(const char* path);

// Check if config file has changed, reload if so
void config_watch(struct global* global);

// Validate key & raw value string, returns 1 if valid, 0 if invalid
int config_validate(const char* key, const char* raw_val, SchemaError* err_out);

#endif
