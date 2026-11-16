// =============================================================
// Smart Garden Free Tier - ESP32 + DHT11
// - Tiap 5 detik : kirim suhu & kelembapan ke MQTT Avisha (realtime dashboard)
// - Tiap 2 menit : simpan ke Supabase (riwayat untuk grafik)
// Library: "DHT sensor library" (Adafruit) + "Adafruit Unified Sensor" + "PubSubClient"
// Wiring DHT11: VCC -> 3V3, GND -> GND, DATA -> GPIO 4
// =============================================================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "secrets.h"

#define DHTPIN 4
#define DHTTYPE DHT11

const unsigned long MQTT_INTERVAL = 5000;
const unsigned long DB_INTERVAL   = 120000;  // 2 menit = 720 baris/hari

const String TOPIC_TELEMETRI = String(TOPIC_PREFIX) + "dht/telemetri";
const String TOPIC_STATUS    = String(TOPIC_PREFIX) + "dht/status";

DHT dht(DHTPIN, DHTTYPE);
WiFiClient netMqtt;
PubSubClient mqtt(netMqtt);

float suhu = NAN, kelembapan = NAN;
unsigned long lastMqtt = 0, lastDb = 0, lastRetry = 0;

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("WiFi: menghubungkan ke ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; i++) delay(250);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK, IP: ");
    Serial.println(WiFi.localIP());
  }
}

void connectMqtt() {
  if (mqtt.connected() || millis() - lastRetry < 5000) return;
  lastRetry = millis();
  String id = "esp32-dht-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  // Last Will: broker otomatis kirim "offline" kalau ESP32 mati/putus
  if (mqtt.connect(id.c_str(), MQTT_USER, MQTT_PASS, TOPIC_STATUS.c_str(), 1, true, "offline")) {
    mqtt.publish(TOPIC_STATUS.c_str(), "online", true);
    Serial.println("MQTT OK");
  } else {
    Serial.print("MQTT gagal, rc=");
    Serial.println(mqtt.state());
  }
}

bool bacaSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("DHT11 gagal dibaca, cek kabel DATA ke GPIO 4");
    return false;
  }
  suhu = t;
  kelembapan = h;
  return true;
}

void kirimSupabase() {
  WiFiClientSecure tls;
  tls.setInsecure();  // shortcut: sertifikat tidak diverifikasi; pasang root CA bila dipakai serius
  HTTPClient http;
  http.setTimeout(5000);
  if (!http.begin(tls, String(SUPABASE_URL) + "/rest/v1/sensor_logs")) return;
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SUPABASE_KEY);
  http.addHeader("x-device-key", DEVICE_KEY);
  http.addHeader("Prefer", "return=minimal");
  String body = "{\"suhu\":" + String(suhu, 1) + ",\"kelembapan\":" + String(kelembapan, 1) + "}";
  int code = http.POST(body);
  Serial.print("Supabase HTTP ");
  Serial.println(code);  // 201 = tersimpan, 401/403 = kunci/policy salah
  if (code != 201) Serial.println(http.getString());
  http.end();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  connectWiFi();
}

void loop() {
  connectWiFi();
  if (WiFi.status() != WL_CONNECTED) { delay(1000); return; }
  connectMqtt();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastMqtt >= MQTT_INTERVAL) {
    lastMqtt = now;
    if (bacaSensor()) {
      String json = "{\"suhu\":" + String(suhu, 1) + ",\"kelembapan\":" + String(kelembapan, 1) + "}";
      if (mqtt.connected()) mqtt.publish(TOPIC_TELEMETRI.c_str(), json.c_str());
      Serial.println(json);
    }
  }
  if (now - lastDb >= DB_INTERVAL && !isnan(suhu)) {
    lastDb = now;
    kirimSupabase();
  }
}
