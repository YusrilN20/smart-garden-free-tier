# 🌱 Smart Garden — Free Tier (ESP32 + DHT11)

Proyek belajar IoT: **ESP32 membaca suhu & kelembapan dari DHT11**, mengirimnya ke internet,
lalu ditampilkan di **dashboard web** (angka realtime + grafik) yang dilindungi **login**.
Web, database, dan hosting memakai paket **gratis**; hanya broker MQTT yang berbayar
(lihat tabel di bawah, ada alternatif gratisnya).

> Ini versi sederhana untuk belajar. Sengaja tidak ada kontrol relay, mode otomatis, LoRa,
> manajemen user, atau export data. Bagian "Tantangan" di bawah adalah bahan riset kamu sendiri.

## Arsitektur

```
DHT11 ──> ESP32 ──(WiFi)──┬── MQTT (tiap 5 dtk) ──> Broker Avisha ──WSS──> Dashboard (realtime)
                          └── HTTPS (tiap 2 mnt) ─> Supabase (tabel) ────> Dashboard (grafik)
Dashboard = HTML + JS statis di Vercel, login memakai Supabase Auth
```

| Layanan | Fungsi | Biaya |
|---|---|---|
| [Avisha](https://broker.avisha.id) | Broker MQTT (data realtime) | **berbayar** — beli paket (mis. 1 tahun), tiap orang dapat username sendiri |
| [Supabase](https://supabase.com) | Database riwayat + login | 500 MB DB |
| [GitHub](https://github.com) | Simpan kode | gratis |
| [Vercel](https://vercel.com) | Hosting dashboard | Hobby |

## Isi folder

```
firmware/esp32_dht11/   program ESP32 (Arduino IDE)
supabase/schema.sql     tabel, keamanan (RLS), hapus data lama otomatis
web/                    dashboard: index.html, app.js, style.css, config.js
```

## Bahan

- ESP32 DevKit (30/38 pin), sensor DHT11 (modul 3 pin), kabel jumper, kabel USB data.

| DHT11 | ESP32 |
|---|---|
| VCC / + | 3V3 |
| GND / − | GND |
| DATA / S | GPIO 4 |

---

## Langkah 1 — Broker MQTT

Proyek ini memakai broker **Avisha**. Brokernya **berbayar**: kamu beli paket langganan
(misalnya 1 tahun), lalu kamu mendapat **username sendiri** beserta password MQTT.
Jadi username di contoh ini tidak bisa dipakai bersama — tiap orang punya akunnya masing-masing.

Tidak mau berlangganan? Pakai broker gratis, kodenya tidak perlu banyak diubah:

| Pilihan | Catatan |
|---|---|
| **HiveMQ Cloud** (gratis, 100 perangkat) | butuh akun; dapat alamat sendiri + username/password. Port ESP32 **8883 (TLS)**, browser `wss://<alamat>:8884/mqtt`. Di firmware ganti `WiFiClient` → `WiFiClientSecure` + `setInsecure()` |
| **broker.emqx.io / test.mosquitto.org** (publik, tanpa akun) | paling cepat untuk coba-coba, tapi **siapa pun bisa ikut membaca datamu**. Jangan dipakai untuk data sungguhan. Username/password diisi string kosong `''` |

Sisa langkah di bawah memakai Avisha. Kalau kamu pakai broker lain, cukup ganti
`MQTT_HOST`, `MQTT_PORT`, username/password, dan `mqtt_url` di tabel `app_config`.

1. Setelah paket aktif, catat **username** dan **password** MQTT milikmu.
2. Topik wajib diawali username kamu, misalnya `namauser/dht/telemetri`. Jadi prefix = `namauser/`.
3. Port: ESP32 memakai **1883** (TCP), browser memakai **8084** (WebSocket aman, `wss://broker.avisha.id:8084/mqtt`).

## Langkah 2 — Supabase
1. Buat project baru.
2. **SQL Editor** → tempel isi `supabase/schema.sql` → **Run**.
3. Buat kunci perangkat acak (≥ 24 karakter, misalnya dari password generator), lalu jalankan bagian 7
   di `schema.sql` (hapus tanda `--`, ganti nilai `<...>`).
4. **Authentication → Sign In / Providers** → matikan **Allow new users to sign up**.
   Ini penting: kalau dibiarkan, siapa saja bisa membuat akun dan membaca dashboard kamu.
5. **Authentication → Users → Add user** → buat akun untuk login (centang *Auto Confirm*).
6. **Project Settings → API**: catat **Project URL** dan **anon / publishable key**.

## Langkah 3 — ESP32
1. Arduino IDE → Board Manager: pasang **esp32 by Espressif**.
2. Library Manager: pasang **DHT sensor library** (Adafruit), **Adafruit Unified Sensor**, **PubSubClient**.
3. Salin `firmware/esp32_dht11/secrets.example.h` menjadi `secrets.h` di folder yang sama, isi semua nilainya.
4. Board: **ESP32 Dev Module** → Upload → buka Serial Monitor **115200**.

Contoh output yang benar:
```
WiFi OK, IP: 192.168.1.23
MQTT OK
{"suhu":28.4,"kelembapan":71.0}
Supabase HTTP 201
```

| Pesan | Artinya |
|---|---|
| `DHT11 gagal dibaca` | kabel DATA bukan di GPIO 4 / VCC-GND tertukar |
| `MQTT gagal, rc=5` | username/password Avisha salah |
| `Supabase HTTP 401` / `403` / `42501` | `SUPABASE_KEY` atau `DEVICE_KEY` tidak cocok dengan tabel `device_keys` |
| `Supabase HTTP 400` | nilai sensor di luar batas (lihat `check` di schema) |

## Langkah 4 — Dashboard
1. Isi `web/config.js` dengan Project URL dan anon key Supabase.
2. Coba lokal: buka folder `web` dengan ekstensi *Live Server* (VS Code), atau `npx serve web`.
3. Push repo ke GitHub kamu → Vercel **Add New → Project** → import repo →
   **Root Directory: `web`**, Framework: **Other** → Deploy.
4. Buka alamat Vercel → login → angka berubah tiap ±5 detik, grafik terisi tiap 2 menit.

## Keamanan (kenapa aman walau kodenya publik)
- **anon key Supabase memang publik.** Yang menjaga data adalah RLS di `schema.sql`:
  hanya user login yang bisa membaca, hanya ESP32 dengan `x-device-key` benar yang bisa menulis.
- Password Avisha **tidak** ditulis di kode web; dashboard mengambilnya dari tabel `app_config`
  setelah login. `secrets.h` tidak ikut ter-commit (`.gitignore`).
- Jangan pernah commit password WiFi, password broker, atau kunci perangkat.

---

## 📚 Jalur materi & tantangan riset

Kerjakan berurutan. Tiap tahap: baca, coba, catat hasilnya.

1. **Sensor** — Bedanya DHT11 vs DHT22 (akurasi, rentang)? Kenapa DHT11 tidak boleh dibaca lebih cepat dari 1 detik?
2. **MQTT** — Pelajari *topic*, *QoS*, *retained message*, dan *Last Will* (dipakai di `connectMqtt()` untuk status online/offline). Coba subscribe topikmu memakai **MQTTX**.
3. **REST API** — Bagaimana ESP32 menyimpan data ke Supabase hanya dengan HTTP POST? Baca dokumentasi PostgREST.
4. **Row Level Security** — Baca `schema.sql`. Coba kirim data dengan `DEVICE_KEY` salah: apa yang terjadi, dan kenapa?
5. **Kuota gratis** — Hitung: 1 baris tiap 2 menit = berapa baris/bulan? Kenapa perlu `pg_cron` menghapus data > 30 hari?

Tantangan lanjutan (tidak disediakan kodenya):
- [ ] Tambah sensor kelembapan tanah (soil moisture, pin analog) sampai tampil di dashboard.
- [ ] Tambah relay pompa yang bisa di-ON/OFF dari dashboard. Pikirkan: bagaimana mencegah orang lain mengirim perintah ke topikmu?
- [ ] Mode otomatis: pompa menyala sendiri saat tanah kering.
- [ ] Pilihan rentang grafik (1 jam / 7 hari) dan unduh CSV.
- [ ] Simpan WiFi lewat portal (WiFiManager) supaya tidak perlu upload ulang saat ganti WiFi.
- [ ] Verifikasi sertifikat TLS di ESP32 (ganti `setInsecure()` dengan root CA).

## Lisensi
MIT — silakan dipakai untuk belajar.
