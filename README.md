AI WISE - ESP32 & MPU9250

Proyek ini adalah sistem pemantauan pergerakan (motion tracking) dan deteksi jatuh (fall detection) real-time berbasis ESP32 dan sensor IMU MPU9250. Perangkat dirancang untuk dipasang secara vertikal pada sabuk pengguna. Data sensor dikirimkan secara real-time ke Web Dashboard (HTML, CSS, JS) menggunakan protokol Server-Sent Events (SSE) melalui jaringan WiFi.

🚀 Panduan Memulai & Konfigurasi WiFi
Sebelum mengunggah kode ke ESP32, sesuaikan kredensial jaringan WiFi yang akan digunakan agar perangkat dapat terhubung.
1. Buka folder ESP32 lalu ke aiwise.ino di Arduino IDE.
2. Cari baris berikut di bagian atas kode:
    ```cpp
    const char* ssid     = "NAMA_WIFI_KAMU_DISINI";
    const char* password = "PASSWORD_WIFI_KAMU_DISINI";
4. Pastikan laptop/smartphone yang digunakan untuk membuka Web Dashboard berada dalam satu jaringan WiFi yang sama dengan Perangkat ESP32.
5. Setelah di-upload, buka Serial Monitor (Baudrate: 115200) untuk melihat IP Address yang didapatkan oleh ESP32 (contoh: 192.168.1.5). Masukkan IP tersebut ke browser Anda untuk membuka dashboard.

📚 Library Arduino yang Dibutuhkan
Pastikan library berikut sudah terinstal di Arduino IDE Anda:
1. Wire.h (Bawaan)
2. MPU9250_asukiaaa
3. WiFi.h (Bawaan ESP32)
4. ESPAsyncWebServer
5. ArduinoJson

🚨 Prosedur Kalibrasi & Arti Indikator LED
Untuk mendapatkan akurasi sensor yang maksimal, perangkat menerapkan sistem kalibrasi hulu (hardware mapping). SOP penyalaan alat wajib mengikuti panduan ini karena posisi sensor dipasang vertikal (Sumbu Y fisik menghadap ke atas):


📊 Standar Satuan dan Konvensi Sudut (Standar Internasional)
Sistem koordinat 3D pada alat ini telah disesuaikan agar patuh pada regulasi matematika spasial Right-Hand Rule (Z-Up) untuk mempermudah integrasi dengan platform navigasi atau robotika.
1. Standar Rotasi / Sudut (Euler Angles)
   Nilai sudut awal saat alat selesai dikalibrasi menghadap ke depan adalah (0, 0, 0).
   - PITCH (Rotasi Sumbu Depan-Belakang):
     - Membungkuk / Menunduk ke Depan = Nilai Negatif (-)
     - Tengadah / Condong ke Belakang = Nilai Positif (+)
   - ROLL (Rotasi Miring Samping):
     - Miring ke Kanan = Nilai Positif (+)
     - Miring ke Kiri = Nilai Negatif (-)
   - YAW (Rotasi Arah Hadap / Kompas):
     - Putar ke Kiri (Counter-Clockwise / CCW) = Nilai bertambah ke Positif (+) 0° -> 90° -> 180°
     - Putar ke Kanan (Clockwise / CW) = Nilai berkurang ke Negatif (-) 0° -> -90° -> -180°
2. Standar Percepatan (Linear Acceleration)
   Menggunakan satuan Standar Internasional m/s² (G-Force dikalikan 9.80665).
   - Sumbu Z (Vertikal): Saat pengguna berdiri tegak, Sumbu Z akan membaca gaya gravitasi murni sebesar -9.8 m/s². Jika terjadi hentakan jatuh mendadak, nilai resultan total percepatan (totalAcc) akan melonjak melewati batas aman (> 20.0 m/s²) dan memicu status "JATUH!".
   - Sumbu X & Y: Merepresentasikan percepatan linear translasi ke arah depan-belakang dan samping kanan-kiri.
3. Kecepatan Sudut (Giroskop)
   Menggunakan satuan derajat per detik (deg/s). Membaca seberapa cepat sentakan atau laju rotasi yang sedang terjadi pada sumbu X, Y, dan Z secara real-time sebelum distabilkan menjadi sudut Euler.

🛠️ Fitur Tombol Reset Posisi (Hold 5 Detik)
Perangkat dilengkapi dengan tombol kalibrasi dinamis (hardware button) yang terhubung pada pin BUTTON_PIN (D3).
- Cara Penggunaan: Jika posisi sudut di dashboard bergeser akibat drift sensor, tekan dan tahan tombol selama 2 detik.
- Indikator: LED Hijau akan mati sementara dan LED Merah akan berkedip cepat selama 300 milidetik sebagai tanda kalibrasi sukses.
- Hasil: Posisi orientasi tubuh Anda saat tombol ditekan tersebut akan dipaksa kembali menjadi titik awal Roll: 0.0, Pitch: 0.0, Yaw: 0.0 tanpa perlu me-reboot ulang ESP32.
- Catatan: Jika tombol ditekan kurang dari 2 detik, sistem tidak akan melakukan reset (anti-bocor/anti-peleset).

📂 Struktur Repositori
- ESP32/aiwise.ino : Berisi kode program utama Arduino (.ino) untuk ESP32.
- Web Dashboard : Berisi file front-end (index.html, style.css, script.js) untuk menampilkan visualisasi data grafis dan status jatuh pengguna.
