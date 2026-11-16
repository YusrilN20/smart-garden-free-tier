// Salin file ini menjadi "secrets.h" lalu isi. secrets.h TIDAK ikut ke GitHub (.gitignore).
#pragma once

#define WIFI_SSID      "nama-wifi"
#define WIFI_PASS      "password-wifi"

// Broker Avisha (MQTT TCP untuk ESP32)
#define MQTT_HOST      "broker.avisha.id"
#define MQTT_PORT      1883
#define MQTT_USER      "user-avisha"
#define MQTT_PASS      "password-avisha"
#define TOPIC_PREFIX   "user-avisha/"   // harus sama dengan topic_prefix di tabel app_config

// Supabase > Project Settings > API
#define SUPABASE_URL   "https://xxxxxxxx.supabase.co"
#define SUPABASE_KEY   "anon-atau-publishable-key"
#define DEVICE_KEY     "kunci-acak-minimal-24-karakter"  // sama dengan isi tabel device_keys
