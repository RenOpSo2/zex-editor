#include "file.h"
#include "nodes.h"

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

constexpr std::uint64_t kMaxFileSize = 64u * 1024u * 1024u;

class FileDescriptor {
public:
    explicit FileDescriptor(int fd = -1) : fd_(fd) {}
    ~FileDescriptor() { reset(); }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept : fd_(other.fd_)
    {
        other.fd_ = -1;
    }

    FileDescriptor& operator=(FileDescriptor&& other) noexcept
    {
        if (this != &other) {
            reset(other.fd_);
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const { return fd_; }
    bool valid() const { return fd_ >= 0; }

    void reset(int fd = -1)
    {
        if (fd_ >= 0) {
            ::close(fd_);
        }
        fd_ = fd;
    }

private:
    int fd_;
};

class TempFileGuard {
public:
    explicit TempFileGuard(const char* path) : path_(path) {}
    ~TempFileGuard()
    {
        if (armed_ && path_ && path_[0] != '\0') {
            ::unlink(path_);
        }
    }

    TempFileGuard(const TempFileGuard&) = delete;
    TempFileGuard& operator=(const TempFileGuard&) = delete;

    void disarm() { armed_ = false; }

private:
    const char* path_;
    bool armed_ = true;
};

static bool write_all(int fd, const void* src, size_t len)
{
    const auto* bytes = static_cast<const char*>(src);
    size_t off = 0;

    while (off < len) {
        ssize_t n = ::write(fd, bytes + off, len - off);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (n == 0) {
            return false;
        }
        off += static_cast<size_t>(n);
    }

    return true;
}

/*
 * file_validate_path — Canonicalize *path* and verify it names a regular file.
 *
 * Uses realpath() to resolve symlinks and ".." segments, then stat() to
 * confirm the result is a regular file (S_ISREG).  This prevents path-
 * traversal and device-file exploitation before any fd is opened.
 */
static enum result file_validate_path(const char* path, char resolved[PATH_MAX])
{
    if (!path || path[0] == '\0') {
        return err;
    }

    if (realpath(path, resolved)) {
        struct stat st;
        if (stat(resolved, &st) == -1 || !S_ISREG(st.st_mode)) {
            return err;
        }
        return ok;
    }

    if (errno != ENOENT) {
        return err;
    }

    /* If the file doesn't exist yet, validate the parent directory. */
    std::array<char, PATH_MAX> tmp{};
    int n = std::snprintf(tmp.data(), tmp.size(), "%s", path);
    if (n < 0 || static_cast<size_t>(n) >= tmp.size()) {
        return err;
    }

    char* dir = ::dirname(tmp.data());
    std::array<char, PATH_MAX> dir_resolved{};
    if (!realpath(dir, dir_resolved.data())) {
        return err;
    }

    struct stat st_dir;
    if (stat(dir_resolved.data(), &st_dir) == -1 || !S_ISDIR(st_dir.st_mode)) {
        return err;
    }

    /* Reconstruct resolved path: <dir_resolved>/<basename> */
    n = std::snprintf(tmp.data(), tmp.size(), "%s", path);
    if (n < 0 || static_cast<size_t>(n) >= tmp.size()) {
        return err;
    }
    char* bname = ::basename(tmp.data());

    n = std::snprintf(resolved, PATH_MAX, "%s/%s", dir_resolved.data(), bname);
    if (n < 0 || n >= PATH_MAX) {
        return err;
    }

    return ok;
}

} // namespace

extern "C" {

/*
 * file_read — Load the file at *path* into *pgb*.
 *
 * Adds path canonicalization via file_validate_path() and opens the
 * resolved path with O_NOFOLLOW to guard against a symlink swap between
 * the validate call and open().
 *
 * Enforces MAX_FILE_SIZE via fstat() before entering the read loop.
 */
enum result file_read(struct paged_gap_buffer* pgb, const char* path, Arena* arena)
{
    if (!pgb || !arena || !path || path[0] == '\0') {
        return err;
    }

    std::array<char, PATH_MAX> resolved{};
    if (file_validate_path(path, resolved.data()) != ok) {
        return err;
    }

    /* O_NOFOLLOW: refuse to follow a symlink at the final path component. */
    FileDescriptor fd(::open(resolved.data(), O_RDONLY | O_NOFOLLOW));
    if (!fd.valid()) {
        return err;
    }

    /* Verify size before allocation to prevent memory exhaustion (NX-003) */
    struct stat st;
    if (fstat(fd.get(), &st) == -1) {
        return err;
    }
    if (static_cast<std::uint64_t>(st.st_size) > kMaxFileSize) {
        return err;
    }

    std::array<char, 4096> buffer{};
    while (true) {
        ssize_t bytes_read = ::read(fd.get(), buffer.data(), buffer.size());
        if (bytes_read < 0) {
            if (errno == EINTR) {
                continue;
            }
            return err;
        }
        if (bytes_read == 0) {
            break;
        }

        for (ssize_t i = 0; i < bytes_read; ++i) {
            pgb_insert(pgb, buffer[static_cast<size_t>(i)], arena);
        }
    }

    return ok;
}

/*
 * file_write — Atomically persist *pgb* to *path*.
 *
 * Strategy (NX-002):
 *   1. Canonicalize the target path and create a sibling temp file
 *      ("<path>.XXXXXX") with mkstemp().
 *   2. Write all gap-buffer pages into the temp file.
 *   3. fsync() to flush kernel buffers to disk.
 *   4. rename() the temp file over the target — POSIX guarantees this
 *      is atomic on the same filesystem, so readers never see a partial
 *      file and a crash never destroys the original.
 */
enum result file_write(const char* path, struct paged_gap_buffer* pgb)
{
    if (!path || path[0] == '\0' || !pgb) {
        return err;
    }

    std::array<char, PATH_MAX> resolved{};
    if (file_validate_path(path, resolved.data()) != ok) {
        return err;
    }

    std::array<char, PATH_MAX> tmp_path{};
    int n = std::snprintf(tmp_path.data(), tmp_path.size(), "%s.XXXXXX", resolved.data());
    if (n < 0 || static_cast<size_t>(n) >= tmp_path.size()) {
        return err;
    }

    /* mkstemp creates the file with 0600 permissions. */
    FileDescriptor fd(::mkstemp(tmp_path.data()));
    if (!fd.valid()) {
        return err;
    }

    TempFileGuard cleanup(tmp_path.data());

    for (struct page* p = pgb->head; p; p = p->next) {
        if (p->gap_start > 0) {
            if (!write_all(fd.get(), p->data, p->gap_start)) {
                return err;
            }
        }

        uint32_t right = PAGE_CAPACITY - p->gap_end;
        if (right > 0) {
            if (!write_all(fd.get(), p->data + p->gap_end, right)) {
                return err;
            }
        }
    }

    /* Flush to disk before exposing the file under the real name. */
    if (fsync(fd.get()) == -1) {
        return err;
    }

    /* Atomic promotion: replaces the target only after a successful write. */
    if (rename(tmp_path.data(), resolved.data()) == -1) {
        return err;
    }

    cleanup.disarm();
    return ok;
}

} // extern "C"
