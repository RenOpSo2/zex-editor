#include "file.h"
#include "nodes.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * file_validate_path — Canonicalize *path* and verify it names a regular file.
 *
 * Uses realpath() to resolve symlinks and ".." segments, then stat() to
 * confirm the result is a regular file (S_ISREG).  This prevents path-
 * traversal and device-file exploitation before any fd is opened.
 *
 * Addresses: NX-001, NX-002
 */
static enum result file_validate_path(const char* path, char resolved[PATH_MAX]) {
    char* rp = realpath(path, resolved);
    if (!rp) return err;

    struct stat st;
    if (stat(resolved, &st) == -1) return err;
    if (!S_ISREG(st.st_mode)) return err;   /* reject devices, FIFOs, sockets */

    return ok;
}

/* Maximum file size the editor will load (64 MiB). */
#define MAX_FILE_SIZE (64 * 1024 * 1024)

/*
 * file_read — Load the file at *path* into *pgb*.
 *
 * Adds path canonicalization via file_validate_path() and opens the
 * resolved path with O_NOFOLLOW to guard against a symlink swap between
 * the validate call and open().
 *
 * Enforces MAX_FILE_SIZE via fstat() before entering the read loop.
 *
 * Addresses: NX-001, NX-003
 */
enum result file_read(struct paged_gap_buffer* pgb, const char* path, Arena* arena) {
    char resolved[PATH_MAX];
    if (file_validate_path(path, resolved) != ok) return err;

    /* O_NOFOLLOW: refuse to follow a symlink at the final path component. */
    int fd = open(resolved, O_RDONLY | O_NOFOLLOW);
    if (fd == -1) return err;

    /* Verify size before allocation to prevent memory exhaustion (NX-003) */
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return err;
    }
    if (st.st_size > MAX_FILE_SIZE) {
        close(fd);
        return err;
    }

    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            pgb_insert(pgb, buffer[i], arena);
        }
    }
    close(fd);

    if (bytes_read == -1) return err;
    return ok;
}

/*
 * file_write — Atomically persist *pgb* to *path*.
 *
 * Strategy (NX-002):
 *   1. Create a sibling temp file ("<path>.XXXXXX") with mkstemp().
 *      mkstemp uses mode 0600 — no world-readable data leak (fixes
 *      the hardcoded 0644 issue from the old code).
 *   2. Write all gap-buffer pages into the temp file.
 *   3. fsync() to flush kernel buffers to disk.
 *   4. rename() the temp file over the target — POSIX guarantees this
 *      is atomic on the same filesystem, so readers never see a partial
 *      file and a crash never destroys the original.
 *
 * Addresses: NX-002
 */
enum result file_write(const char* path, struct paged_gap_buffer* pgb) {
    /* Build a temp path adjacent to the target file. */
    char tmp_path[PATH_MAX];
    int n = snprintf(tmp_path, sizeof(tmp_path), "%s.XXXXXX", path);
    if (n < 0 || (size_t)n >= sizeof(tmp_path)) return err;

    int fd = mkstemp(tmp_path);   /* creates with 0600 — no world-read leak */
    if (fd == -1) return err;

    struct page* p = pgb->head;
    while (p) {
        if (p->gap_start > 0) {
            if (write(fd, p->data, p->gap_start) != (ssize_t)p->gap_start) {
                close(fd); unlink(tmp_path); return err;
            }
        }
        uint32_t right = PAGE_CAPACITY - p->gap_end;
        if (right > 0) {
            if (write(fd, p->data + p->gap_end, right) != (ssize_t)right) {
                close(fd); unlink(tmp_path); return err;
            }
        }
        p = p->next;
    }

    /* Flush to disk before exposing the file under the real name. */
    if (fsync(fd) == -1) { close(fd); unlink(tmp_path); return err; }
    close(fd);

    /* Atomic promotion: replaces the target only after a successful write. */
    if (rename(tmp_path, path) == -1) { unlink(tmp_path); return err; }

    return ok;
}
