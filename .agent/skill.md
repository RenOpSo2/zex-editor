# Zex Editor — AI Agent Skill & Business Context Guide

Dokumen ini berisi panduan konteks bisnis, arsitektur sistem, domain rules, dan standar rekayasa software untuk AI Agent yang bekerja di codebase **Zex Editor**.

---

## 1. 🎯 Konteks Bisnis & Visi Produk (Business Context)

### Apa itu Zex Editor?
**Zex** adalah text editor berbasis terminal (TUI - Terminal User Interface) modern, ultra-cepat, dan efisien yang dibangun menggunakan bahasa C murni (`gnu99`). Editor ini dirancang untuk pengguna terminal/CLI yang menginginkan performa native sekelas `vim`/`nano` namun dengan kebiasaan shortcut yang ramah dan intuitif seperti editor modern (VS Code).

### Target Pengguna & Value Proposition:
1. **Developer & DevOps Engineer**: Membutuhkan editor teks instan di server remote/SSH atau environment lokal tanpa overhead JVM, browser engine (Electron), atau runtime berat.
2. **Pengguna Terminal Modern**: Pengguna yang terbiasa dengan shortcut standar industri (Ctrl+S untuk save, Ctrl+C/V/X clipboard, Ctrl+Z/U undo, Ctrl+Y redo, Ctrl+F search, Shift+Arrow selection).
3. **Efisiensi Sumber Daya**: Komputer dengan spesifikasi terbatas atau kontainer minimalis yang memerlukan konsumsi RAM sangat rendah (hitungan Kilobyte/Megabyte, bukan Gigabyte).

### Filosofi Produk (Core Values):
* **Zero External Dependencies**: Hanya bergantung pada C standard library (`libc`) dan POSIX syscalls. Tidak menggunakan library TUI berat (seperti ncurses) demi menjaga portabilitas dan kontrol penuh atas rendering.
* **Low Latency & High Throughput**: Pengetikan dan navigasi harus bebas dari input lag.
* **Predictable & Safe Memory**: Menggunakan Arena Allocator untuk alokasi memori internal sehingga bebas memory leak dan terhindar dari fragmentasi memori.

---

## 2. 🏛️ Arsitektur Sistem & Core Invariants

### 2.1 Struktur Data Teks: Paged Gap Buffer
File: `src/nodes.c`, `src/nodes.h`, `src/global.h`
* Teks disimpan dalam **Paged Gap Buffer** (koleksi linked list dari `struct page` berukuran 4096 bytes / `PAGE_CAPACITY`).
* Setiap page memiliki gap buffer sendiri (`gap_start` dan `gap_end`).
* **Invarian**:
  * Penambahan karakter dilakukan di `gap_start` pada `active_page`.
  * Jika gap pada halaman penuh, buffer membagi halaman (*page split*) atau mengalokasikan page baru dari Arena.
  * Operasi kursor (linear position vs page offset) harus sinkron antara baris visual dan byte internal buffer.

### 2.2 Manajemen Memori: Custom Arena Allocator
File: `libmemory/arena.c`, `libmemory/arena.h`
* Alokasi heap dikelola oleh `Arena` (kapasitas default 16MB / `arena_capacity`).
* Alokasi node dan buffer halaman mengambil chunk dari arena dengan *bump allocation*.
* **Invarian**: Jangan gunakan `malloc`/`free` sembarangan untuk alokasi per-karakter di dalam hot-loop; selalu prioritaskan alokasi melalui `arena_alloc()` atau struktur data lokal.

### 2.3 Terminal & Double Buffering Renderer
File: `src/term.c`, `src/render_buffer.c`, `src/draw.c`
* Mode terminal diubah ke *raw mode* via POSIX `termios`.
* Seluruh frame digambar terlebih dahulu ke render buffer dalam memori sebelum di-*flush* satu kali ke stdout menggunakan ANSI escape sequences.
* Mencegah screen tearing / flicker pada terminal berkecepatan tinggi.

### 2.4 Sistem Konfigurasi & Live Watcher
File: `src/config.c`, `src/config.h`, `api/api.md`
* Konfigurasi dibaca secara berurutan dengan prioritas:
  1. CLI Flags (`--tabsize`, `--mouse`, dsb) *(Prioritas Tertinggi)*
  2. File lokal `./zex.json`
  3. File global `~/.zexrc` *(Prioritas Terendah)*
* Didukung oleh file watcher berbasis polling `stat()` `mtime` untuk live auto-reload konfigurasi tanpa restart editor.

---

## 3. 📂 Peta Direktori & Modul Codebase

```
zex-editor/
├── .agent/
│   └── skill.md          # Dokumen panduan AI agent & konteks bisnis (file ini)
├── api/
│   └── api.md            # Dokumentasi user guide konfigurasi Zex
├── libmemory/            # Modul Arena Memory Allocator
│   ├── arena.c
│   └── arena.h
├── src/                  # Kode sumber utama editor Zex
│   ├── cmd.c / .h        # Eksekusi command bar internal
│   ├── config.c / .h     # JSON parser & config auto-reload
│   ├── draw.c / .h       # Logika render UI, gutter, cursor, status bar
│   ├── editor.c / .h     # Init, main update loop, cleanup state
│   ├── file.c / .h       # File I/O (load, save, backup)
│   ├── global.h          # Definisi struct global, page, action, term, macros
│   ├── input.c / .h      # Keybindings dispatcher, escape sequence parsing
│   ├── main.c           # Entry point & CLI argument processing
│   ├── nodes.c / .h      # Paged Gap Buffer, undo/redo stack, search engine
│   ├── render_buffer.c   # Buffer output layar ke terminal
│   ├── syntax.c / .h     # Highlighting syntax tokenizer (C/C++)
│   └── term.c / .h       # POSIX terminal raw mode control
├── tests/                # Test suite dan unit tests
├── Makefile              # Build automation script
└── README.md             # Dokumentasi umum project
```

---

## 4. 📋 Aturan Bisnis & Rekayasa (Engineering Rules for AI Agent)

Ketika memodifikasi, menambah fitur, atau memecahkan bug pada codebase ini, AI Agent **WAJIB** mematuhi aturan berikut:

1. **Jaga Prinsip Zero-Dependency**:
   * Jangan menambahkan dependency library eksternal (seperti `ncurses`, `glib`, `cjson`, dll).
   * Semua parsing JSON, rendering ANSI, dan struktur data harus tetap mandiri dan self-contained dalam C standar.

2. **Kompatibilitas Standar C (gnu99 / C99)**:
   * Kode harus selalu dapat di-compile dengan `gcc -std=gnu99 -Wall -Wextra`.
   * Hindari fitur non-portabel yang memecahkan kompatibilitas Linux / POSIX.

3. **Perhatikan Efisiensi Kinerja & Hot Paths**:
   * Fungsi di `src/draw.c`, `src/nodes.c`, dan `src/input.c` dieksekusi pada setiap penekanan tombol. Hindari operasi $O(N^2)$ pada linear scan buffer saat menggambar layar.
   * Manfaatkan lookup berbasis page dan pointer gap buffer.

4. **Keamanan Memori & Boundary Checking**:
   * Selalu validasi batas buffer (`PAGE_CAPACITY`, `MAX_SEARCH_QUERY_LEN`, `MAX_FILEPATH_LEN`, `UNDO_STACK_SIZE`).
   * Cegah buffer overflow pada string formatting terminal.

5. **Integritas Format Kode & Testing**:
   * Jalankan `make` untuk memastikan binary ter-compile bersih tanpa warning.
   * Jalankan `make test` untuk memvalidasi test suite tidak regresi.
   * Jalankan `make format` (menggunakan `clang-format`) sebelum menyelesaikan perubahan kode.

---

## 5. 🛠️ Command Standar Proyek

* **Build Binary**: `make` (hasil binary berada di `bin/zex`)
* **Jalankan Editor**: `make run` atau `./bin/zex <nama_file>`
* **Jalankan Test**: `make test`
* **Format Kode**: `make format`
* **Clean Build**: `make clean`
