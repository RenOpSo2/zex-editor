#include "cmd.h"
#include "file.h"
#include "nodes.h"
#include <string.h>

enum result cmd_openfile(struct paged_gap_buffer* pgb, const char* path, Arena* arena) {
    pgb_clear(pgb);
    return file_read(pgb, path, arena);
}

enum result cmd_savefile(struct paged_gap_buffer* pgb, const char* path) {
    return file_write(path, pgb);
}

enum result cmd_exec(struct global* global, struct paged_gap_buffer* cmd_buf) {
    char buf[buf_capacity];
    char* option;
    pgb_to_str(buf, sizeof(buf), cmd_buf);
    pgb_clear(&global->msg);

    uint32_t i = 0;
    while (buf[i] != ' ' && buf[i] != '\0') {
        i++;
    }
    if (buf[i] == ' ') {
        buf[i++] = '\0';
    }
    option = buf + i;

    if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0 || strcmp(buf, "q") == 0) {
        return err;
    } else if (strcmp(buf, "open") == 0) {
        if (cmd_openfile(&global->text, option, &global->arena) == ok) {
            pgb_replace_str(&global->msg, "open succeeded.", &global->arena);
        } else {
            pgb_replace_str(&global->msg, "open failed.", &global->arena);
        }
        return ok;
    } else if (strcmp(buf, "save") == 0) {
        if (cmd_savefile(&global->text, option) == ok) {
            pgb_replace_str(&global->msg, "save succeeded.", &global->arena);
        } else {
            pgb_replace_str(&global->msg, "save failed.", &global->arena);
        }
        return ok;
    } else {
        pgb_replace_str(&global->msg, "command not found.", &global->arena);
        return ok;
    }
}
