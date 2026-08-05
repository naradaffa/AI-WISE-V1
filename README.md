# AI WISE V1

Sistem *wearable motion tracking* dan *fall detection* real-time untuk sabuk pekerja. Berbasis ESP32 dan IMU MPU9250 dengan transmisi data ke Web Dashboard via WiFi (SSE).

---

## 🚀 Persiapan & Koneksi WiFi

Sistem menggunakan IP statis dari router khusus. **Tidak perlu upload ulang kode.**

*   **WiFi SSID:** `M22_C129`
*   **Password:** `90093656`
*   **Urutan Menyalakan:** Nyalakan Router -> Device 1 (kalibrasi) -> Device 2 (kalibrasi) -> Device 3 (kalibrasi) -> Hubungkan Laptop ke jaringan WiFi.

## 🔗 Menjalankan Web Dashboard

*   Buka folder **Web Dashboard**.
*   Buka `index.html` menggunakan **Live Server** di browser.

## 🚨 Kalibrasi & Indikator LED

Pegang alat vertikal (Sumbu Y menghadap atas) saat dinyalakan.

| Tahap | Status LED | Instruksi & Tujuan |
| :--- | :--- | :--- |
| **Booting** | Merah KEDIP 1X, lalu MATI | Tunggu inisialisasi awal. |
| **Tahap 1 (1s)** | Merah NYALA, Hijau MATI | Pegang alat tegak & diam (isolasi gravitasi). |
| **Tahap 2 (5s)** | Merah & Hijau KEDIP Bergantian | Putar alat horizontal 360° (kalibrasi kompas). |
| **Tahap 3 (2s)** | Merah & Hijau NYALA Bersamaan | Hadapkan lurus ke depan, tahan diam (kunci Yaw 0). |
| **Berjalan** | Merah MATI, Hijau NYALA | Pasang di sabuk, alat aktif kirim data. |

## 📊 Standar Pengukuran

*   **PITCH:** Menunduk (-), Tengadah (+)
*   **ROLL:** Miring Kiri (-), Miring Kanan (+)
*   **YAW:** Putar Kiri (+), Putar Kanan (-)
*   **Akselerometer (X, Y):** Translasi horizontal (m/s²).
*   **Akselerometer (Z):** Translasi vertikal (diam = -9.8 m/s²).
*   **Giroskop:** Kecepatan sentakan rotasi (deg/s).

## 🛠️ Tombol Reset Posisi

*   **Fungsi:** Mengembalikan referensi orientasi ke titik 0 (Roll/Pitch/Yaw) tanpa *reboot*.
*   **Cara Pakai:** Tekan dan tahan tombol selama 2 detik.
*   **Indikator Sukses:** LED Hijau mati sementara, LED Merah kedip cepat (300ms).

## 📂 Struktur Repositori

*   **ESP32:** Kode program utama (`.ino`) dan dependensi (`Wire.h`, `MPU9250_asukiaaa`, `WiFi.h`, `ESPAsyncWebServer`, `ArduinoJson`).
*   **Web Dashboard:** File antarmuka visual (`index.html`, `style.css`, `script.js`).
