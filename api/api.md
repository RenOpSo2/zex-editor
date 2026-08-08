# Panduan Konfigurasi Zex Editor (User Guide)

Panduan ini menjelaskan cara melakukan konfigurasi dan menggunakan Config System pada Zex Editor untuk menyesuaikan kenyamanan mengetik Anda.

---

## 📂 Lokasi File Konfigurasi

Zex Editor mendukung pembacaan konfigurasi secara otomatis dari dua lokasi berikut (diutamakan file lokal terlebih dahulu):

1. **Lokal (Project-level)**: `./zex.json` (file `zex.json` di direktori tempat Anda menjalankan editor).
2. **Global (User-level)**: `~/.zexrc` (file `.zexrc` di direktori home pengguna Anda).

---

## ⚙️ Opsi Konfigurasi yang Didukung

Saat ini, Zex Editor mendukung beberapa opsi konfigurasi berikut:

| Nama Kunci | Tipe Data | Nilai Default | Deskripsi |
| :--- | :--- | :--- | :--- |
| `tabsize` | Angka (Number) | `4` | Menentukan lebar visual karakter tab (`\t`) ketika ditampilkan di layar editor (antara 1 s.d 16). |
| `mouse` | Boolean | `true` | Mengaktifkan/menonaktifkan fungsionalitas pendukung mouse (nilai yang valid: `true`, `false`, `1`, `0`). |
| `show_line_numbers` | Boolean | `true` | Menampilkan/menyembunyikan kolom nomor baris di sebelah kiri layar (gutter). |
| `auto_indent` | Boolean | `true` | Mengaktifkan/menonaktifkan indentasi otomatis ke baris baru saat menekan `Enter`. |

---

## 📝 Format File JSON

Format penulisan di dalam file `./zex.json` atau `~/.zexrc` wajib menggunakan standar JSON yang valid. 

**Contoh isi file konfigurasi:**
```json
{
  "tabsize": 4,
  "mouse": true,
  "show_line_numbers": true,
  "auto_indent": true
}
```

---

## 🚀 Penggunaan via CLI (Command Line Interface)

Anda juga bisa menimpa (override) pengaturan konfigurasi secara langsung melalui argumen baris perintah saat membuka Zex Editor. Argumen CLI ini memiliki prioritas tertinggi dibandingkan file JSON.

### Contoh Perintah:

* **Membuka file dengan ukuran tab = 8:**
  ```bash
  ./bin/zex --tabsize 8 nama_file.txt
  ```

* **Mematikan fungsi mouse dan set tab = 2:**
  ```bash
  ./bin/zex --tabsize 2 --mouse false nama_file.txt
  ```

---

## 🔄 Pembaruan Otomatis (Live Watch / Auto-Reload)

Zex Editor memiliki fitur pemantauan file konfigurasi secara langsung.

Jika Anda mengubah pengaturan di dalam file `./zex.json` atau `~/.zexrc` menggunakan aplikasi lain selagi Zex Editor sedang terbuka, Zex Editor akan:
1. Mendeteksi perubahan berkas secara otomatis.
2. Memuat ulang (re-parse) pengaturan baru tanpa perlu menutup editor.
3. Menampilkan pesan `"Configuration reloaded automatically."` pada status bar bagian bawah layar.
