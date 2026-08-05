# AI WISE V1

Proyek ini adalah sistem pemantauan pergerakan (*motion tracking*) dan deteksi jatuh (*fall detection*) *real-time* berbasis ESP32 dan sensor IMU MPU9250. Perangkat dirancang secara *wearable* dengan daya baterai terintegrasi, dipasang secara vertikal pada sabuk pengguna. Data sensor dikirimkan secara *real-time* ke Web Dashboard (HTML, CSS, JS) menggunakan protokol *Server-Sent Events* (SSE) melalui jaringan WiFi.

## 🚀 Panduan Memulai & Koneksi WiFi

Sistem ini menggunakan 1 modem (router) khusus dan perangkat sudah diisi dengan program utama. Anda tidak perlu mengunggah ulang kode. Untuk memastikan IP Address setiap alat tidak berubah-ubah, wajib mengikuti urutan penyalaan berikut:

1. **Nyalakan Modem Khusus:** Pastikan modem menyala dan memancarkan jaringan WiFi dengan kredensial berikut:
   * **SSID:** `M22_C129`
   * **Password:** `90093656`
2. **Nyalakan dan Kalibrasi Alat Berurutan:** Gunakan saklar pada baterai untuk menyalakan alat satu per satu.
   * Nyalakan **Device 1**, lakukan proses kalibrasi (lihat panduan LED di bawah), lalu tunggu hingga selesai.
   * Setelah Device 1 beroperasi normal, nyalakan **Device 2**, lakukan kalibrasi.
   * Terakhir, nyalakan **Device 3**, dan lakukan kalibrasi.
   *(Proses penyalaan berurutan ini memastikan DHCP modem memberikan IP yang selalu tetap).*
3. **Hubungkan Laptop:** Sambungkan laptop atau komputer pemantau Anda ke jaringan WiFi `M22_C129`.

## 🔗 Menjalankan Web Dashboard

Karena urutan penyalaan di atas membuat alokasi IP Address selalu sama dan statis, Anda bisa langsung membuka antarmuka pemantauan tanpa perlu mengubah konfigurasi kode.

1. Buka folder **Web Dashboard**.
2. Klik kanan file `index.html` lalu pilih **Open with Live Server**.
3. Halaman akan otomatis terbuka di *browser* Anda, dan grafik *dashboard* akan langsung menampilkan pergerakan sensor dari ketiga alat secara *real-time*!

## 🚨 Prosedur Kalibrasi & Arti Indikator LED

Untuk mendapatkan akurasi sensor yang maksimal, perangkat menerapkan sistem kalibrasi hulu (*hardware mapping*). SOP penyalaan alat wajib mengikuti panduan ini karena posisi sensor dipasang vertikal (Sumbu Y fisik menghadap ke atas):

| Tahapan Sistem | Status LED | Instruksi Tindakan Pengguna | Tujuan Sistem |
| :--- | :--- | :--- | :--- |
| **Booting / Power On** | LED Merah KEDIP SEKALI, lalu MATI | Tunggu sesaat hingga alat masuk ke tahap kalibrasi pertama. | Inisialisasi awal sistem dan memuat konfigurasi sensor saat saklar pertama kali ditekan. |
| **Tahap 1 (1 Detik)** | LED Merah NYALA, LED Hijau MATI | Pegang alat dalam posisi berdiri tegak (seperti saat menempel di sabuk) dan diamkan. | Mencari nilai bias awal untuk akselerometer dan mengisolasi gaya gravitasi bumi. |
| **Tahap 2 (5 Detik)** | LED Merah & Hijau Kedip Bergantian | Tetap pegang alat dalam posisi berdiri tegak, lalu putar alat/tubuh Anda 360 derajat secara horizontal (layaknya radar). | Mencari batas nilai minimum-maksimum magnetometer (kompas) pada bidang datar. |
| **Tahap 3 (2 Detik)** | LED Merah & Hijau NYALA BERSAMAAN | Berhenti memutar, hadapkan alat lurus ke DEPAN, dan tahan diam. | Proses *Auto-Zeroing* untuk mengunci arah depan tersebut sebagai sudut Yaw = 0.0. |
| **Tahap Berjalan** | LED Merah MATI, LED Hijau NYALA | Alat siap digunakan di sabuk untuk aktivitas/pengujian. | Sistem beroperasi normal dan mulai mengirimkan data JSON ke *web*. |

## 📊 Standar Satuan dan Konvensi Sudut

Sistem koordinat 3D pada alat ini telah disesuaikan agar patuh pada regulasi matematika spasial *Right-Hand Rule* (Z-Up) untuk mempermudah integrasi dengan platform navigasi atau robotika.

* **PITCH (Rotasi Sumbu Depan-Belakang):** Membungkuk / menunduk ke depan bernilai negatif (-), sedangkan tengadah / condong ke belakang bernilai positif (+).
* **ROLL (Rotasi Miring Samping):** Miring ke kanan bernilai positif (+), sedangkan miring ke kiri bernilai negatif (-).
* **YAW (Rotasi Arah Hadap / Kompas):** Putar ke kiri (CCW) nilai bertambah positif (0 hingga 180). Putar ke kanan (CW) nilai berkurang negatif (0 hingga -180). Sudut awal menghadap ke depan adalah 0.
* **Sumbu Z (Vertikal):** Saat pengguna berdiri tegak, Sumbu Z akan membaca gaya gravitasi murni sebesar -9.8 m/s². Satuan menggunakan m/s² (G-Force dikalikan 9.80665).
* **Sumbu X & Y:** Merepresentasikan percepatan linear translasi ke arah depan-belakang dan samping kanan-kiri.
* **Kecepatan Sudut (Giroskop):** Membaca seberapa cepat sentakan atau laju rotasi yang sedang terjadi pada sumbu X, Y, dan Z dalam satuan derajat per detik (deg/s).

## 🛠️ Fitur Tombol Reset Posisi (Hold 2 Detik)

Perangkat dilengkapi dengan tombol kalibrasi dinamis (*hardware button*) yang terhubung pada pin BUTTON_PIN (D3).

* **Cara Penggunaan:** Tekan dan tahan tombol selama 2 detik untuk merubah referensi titik 0. Menekan kurang dari 2 detik tidak akan memicu *reset*.
* **Indikator:** LED Hijau akan mati sementara dan LED Merah akan berkedip cepat selama 300 milidetik sebagai tanda kalibrasi sukses.
* **Hasil:** Posisi orientasi tubuh Anda saat tombol ditekan akan dipaksa kembali menjadi titik awal (Roll: 0.0, Pitch: 0.0, Yaw: 0.0) tanpa perlu *reboot* ulang ESP32.

## 📂 Struktur Repositori

* **ESP32:** Berisi kode program utama Arduino (`.ino`) dan *library* referensi (`Wire.h`, `MPU9250_asukiaaa`, `WiFi.h`, `ESPAsyncWebServer`, `ArduinoJson`).
* **Web Dashboard:** Berisi file *front-end* (`index.html`, `style.css`, `script.js`) untuk menampilkan visualisasi data grafis dan status jatuh pengguna.
