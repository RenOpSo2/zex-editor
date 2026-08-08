# Struktur Project Zex

Dokumentasi ini menjelaskan struktur direktori dan file dari project Zex dalam bahasa Indonesia.

## 📁 Struktur Utama

```
zex/
├── src/                    # Source code utama editor
├── libmemory/              # Library manajemen memori
├── docs/                   # Dokumentasi project
├── .github/                # Konfigurasi GitHub
├── changelog/              # Riwayat perubahan versi
├── prototype/              # Prototipe dan eksperimen
├── Makefile                # Build system
├── README.md               # Dokumentasi utama
└── test_*.c                # File testing
```

## 📂 Penjelasan Direktori

### `src/` - Source Code Utama
Berisi semua source code untuk editor teks terminal.

#### File-file Utama:
- **`main.c`** - Entry point program, inisialisasi editor
- **`global.h`** - Definisi struktur data global dan konfigurasi
- **`editor.c/h`** - Logika utama editor (init, update, deinit)
- **`nodes.c/h`** - Implementasi Paged Gap Buffer untuk penyimpanan teks
- **`file.c/h`** - Operasi file (baca, tulis, simpan)
- **`input.c/h`** - Penanganan input keyboard dan mouse
- **`draw.c/h`** - Rendering tampilan ke terminal
- **`term.c/h`** - Komunikasi dengan terminal (ANSI escape codes)
- **`syntax.c/h`** - Syntax highlighting untuk kode C
- **`render_buffer.c/h`** - Buffer untuk double-buffering rendering
- **`cmd.c/h`** - Command handling (tidak digunakan di versi ini)

#### Struktur Data Penting:
- **`struct global`** - State utama editor berisi:
  - Terminal info
  - Text buffer (teks utama)
  - Clipboard buffer
  - Message buffer
  - State selection
  - Stack undo/redo
  - State pencarian
  - Arena allocator

- **`struct paged_gap_buffer`** - Struktur data untuk menyimpan teks dengan efisien menggunakan linked list dari pages

- **`struct page`** - Satu page dalam gap buffer dengan kapasitas 4096 bytes

### `libmemory/` - Library Manajemen Memori
Library custom untuk manajemen memori yang efisien.

#### File-file:
- **`arena.h`** - Header untuk Arena Allocator (bump allocator)
- **`arena.c`** - Implementasi Arena Allocator
- **`examples.c`** - Contoh penggunaan Arena Allocator

#### Fitur Arena Allocator:
- Alokasi memori O(1) - sangat cepat
- Tidak perlu free() individual - semua dibebaskan sekaligus
- Perfect untuk text editor buffer, parser, dan game engine
- Mendukung alokasi dengan alignment
- Macro convenience: `arena_new()`, `arena_new_array()`, dll

### `docs/` - Dokumentasi Project
Berisi dokumentasi teknis dan panduan.

#### File-file:
- **`ARCHITECTURE.md`** - Dokumentasi arsitektur sistem
- **`BUG_REPORT.md`** - Template laporan bug
- **`STRUKTUR_PROJECT.md`** - Dokumentasi ini (struktur project)

### `.github/` - Konfigurasi GitHub
Konfigurasi untuk GitHub Actions dan GitHub features.

#### Struktur:
```
.github/
├── workflows/
│   └── codeql.yml          # Workflow CodeQL untuk security scanning
├── codeql/
│   └── codeql-config.yml   # Konfigurasi CodeQL
└── ISSUE_TEMPLATE/
    ├── bug_report.md       # Template laporan bug
    ├── feature_request.md  # Template request fitur
    └── custom.md           # Template custom
```

### `changelog/` - Riwayat Perubahan
Berisi dokumentasi perubahan untuk setiap versi.

#### File-file:
- **`CHANGELOG_0.1.0.txt`** - Versi awal
- **`CHANGELOG_0.2.0.txt`** - Versi 0.2.0
- **`CHANGELOG_0.2.0-stable.txt`** - Versi stabil 0.2.0
- **`CHANGELOG_0.2.1.txt`** - Versi 0.2.1
- **`CHANGELOG_0.2.3.txt`** - Versi 0.2.3 (fix keamanan)

### `prototype/` - Prototipe dan Eksperimen
Berisi kode eksperimental dan prototipe awal.

#### File-file:
- **`main.py`** - Prototipe Python (development awal)

## 📄 File Konfigurasi Utama

### `Makefile`
Build system untuk kompilasi project.

#### Target penting:
- `make` atau `make all` - Build project
- `make run` - Build dan jalankan
- `make test` - Build dan jalankan test
- `make clean` - Hapus build artifacts
- `make format` - Format code dengan clang-format
- `make format-check` - Cek formatting tanpa mengubah file
- `make check` - Static analysis dengan cppcheck

#### Struktur build:
- **Source:** `src/*.c` dan `libmemory/arena.c`
- **Build directory:** `build/`
- **Binary output:** `bin/zex`
- **Compiler:** GCC dengan flags: `-Wall -Wextra -Wpedantic -g -O0 -std=gnu99`

### `README.md`
Dokumentasi utama project yang berisi:
- Deskripsi project
- Fitur-fitur editor
- Cara instalasi
- Panduan penggunaan
- Keybindings
- Arsitektur sistem

### File Konfigurasi Lainnya:
- **`.clang-format`** dan **`.clang_format`** - Konfigurasi clang-format
- **`.gitignore`** - File yang diabaikan oleh Git
- **`CODE_OF_CONDUCT.md`** - Kode etik kontributor
- **`CONTRIBUTING.md`** - Panduan kontribusi
- **`SECURITY.md`** - Kebijakan keamanan
- **`LICENSE`** - Lisensi (GPL v3)
- **`.replit`** dan **`replit.md`** - Konfigurasi Replit

## 🧪 File Testing

### File-file Test:
- **`test_main.c`** - Test utama
- **`test_undo.c`** - Test untuk fitur undo/redo
- **`test_0.2.0.c`** - Test untuk versi 0.2.0
- **`test_main`** - Binary test (hasil build)

## 🏗️ Arsitektur Sistem

### Komponen Utama:

1. **Paged Gap Buffer** - Struktur data untuk penyimpanan teks
   - Menggunakan linked list dari pages
   - Setiap page memiliki gap buffer
   - Efficient untuk insert/delete di posisi manapun

2. **Arena Allocator** - Manajemen memori
   - Bump allocator yang sangat cepat
   - Semua alokasi dalam satu file dibebaskan sekaligus
   - Digunakan untuk node allocation dan buffer management

3. **Render Buffer** - Double-buffering
   - Buffer untuk menyimpan output sebelum ditampilkan
   - Mencegah flickering saat rendering
   - Kapasitas 64KB (2^16 bytes)

4. **Terminal Interface** - Komunikasi dengan terminal
   - ANSI escape codes untuk cursor movement
   - Terminal size detection
   - Raw mode untuk keyboard input

5. **Syntax Highlighting** - Pewarnaan kode
   - Dukungan untuk syntax C
   - Token-based highlighting
   - Menggunakan Python module (opsional)

6. **Undo/Redo System** - History editing
   - Stack-based undo/redo
   - Mendukung insert, delete, dan replace
   - Kapasitas 100 actions

7. **Search System** - Pencarian teks
   - Find next/previous
   - Highlight matches
   - Navigate antar matches

## 🔧 Build Process

### Alur Build:
1. **Source files** → **Compilation** → **Object files** (`.o`)
2. **Object files** → **Linking** → **Executable** (`bin/zex`)

### Dependency Management:
- Otomatis dependency tracking dengan `-MMD -MP`
- File `.d` berisi dependency information
- Rebuild otomatis jika header berubah

## 📊 Flow Data

### Flow Program:
```
main.c
  ↓
editor_init() → Setup terminal, load file, init structures
  ↓
editor_update() → Main loop
  ↓
  - input.c → Handle keyboard input
  - nodes.c → Update text buffer
  - draw.c → Render to render buffer
  - render_buffer.c → Flush to terminal
  ↓
editor_deinit() → Cleanup, save file
```

### Flow Editing:
```
Keyboard Input
  ↓
input.c → Parse input
  ↓
nodes.c → Update paged gap buffer
  ↓
undo_save_* → Save action to undo stack
  ↓
draw.c → Render changes
  ↓
render_buffer.c → Display to terminal
```

## 🎯 Fokus Utama Development

### Area Fokus:
1. **`src/nodes.c`** - Logika teks dan gap buffer
2. **`src/editor.c`** - Logika utama editor
3. **`src/draw.c`** - Rendering dan display
4. **`libmemory/arena.c`** - Manajemen memori

### Area yang Stabil:
- **Terminal interface** (`term.c`)
- **File operations** (`file.c`)
- **Arena allocator** (`libmemory/`)

## 🚀 Extensibility

### Menambah Fitur Baru:
1. **Keybindings baru** → Edit `src/input.c`
2. **Commands baru** → Edit `src/editor.c`
3. **Syntax highlighting baru** → Edit `src/syntax.c`
4. **Struktur data baru** → Edit `src/global.h`

### Testing:
- Tambah test di `test_*.c`
- Build dengan `make test`
- Jalankan test dengan `make test`

## 📝 Catatan Penting

- **Compiler**: GCC dengan C99 standard
- **Build System**: Make
- **Formatter**: clang-format
- **Static Analysis**: cppcheck
- **Security**: CodeQL scanning
- **Lisensi**: GPL v3
- **Versi**: 0.2.3-nxo

## 🔍 Debugging

### Tools:
- **GDB**: Debugging dengan `gdb bin/zex`
- **Valgrind**: Memory leak checking
- **cppcheck**: Static analysis
- **clang-format**: Code formatting

### Logging:
- Tambah `printf` untuk debugging
- Gunakan `make clean && make` untuk rebuild
- Check `build/` untuk object files

---

Dokumentasi ini dibuat untuk memudahkan pemahaman struktur project Zex. Untuk informasi lebih detail, lihat file-file lain di direktori `docs/`.
