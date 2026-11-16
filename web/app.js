// Smart Garden Free Tier - dashboard (tanpa build step)
const { SUPABASE_URL, SUPABASE_KEY } = window.APP_CONFIG;
const db = window.supabase.createClient(SUPABASE_URL, SUPABASE_KEY);
const $ = (id) => document.getElementById(id);
let mqttClient = null;
let chart = null;
let chartTimer = null;

// ---------- Login / logout ----------
$("login-form").addEventListener("submit", async (e) => {
  e.preventDefault();
  $("login-error").textContent = "";
  const { error } = await db.auth.signInWithPassword({
    email: $("email").value.trim(),
    password: $("password").value,
  });
  if (error) $("login-error").textContent = "Login gagal: email atau password salah.";
});

$("logout").addEventListener("click", () => db.auth.signOut());

db.auth.onAuthStateChange((_event, session) => {
  // setTimeout: jangan panggil query Supabase langsung di dalam callback auth (bisa deadlock)
  setTimeout(() => (session ? showApp() : showLogin()), 0);
});

function showLogin() {
  stopApp();
  $("app-view").hidden = true;
  $("login-view").hidden = false;
}

async function showApp() {
  if (!$("app-view").hidden) return;
  $("login-view").hidden = true;
  $("app-view").hidden = false;
  loadChart();
  chartTimer = setInterval(loadChart, 120000); // sama dengan interval simpan ESP32
  const { data, error } = await db.from("app_config").select("*").single();
  if (error || !data) return setStatus("Konfigurasi MQTT belum diisi", "offline");
  connectMqtt(data);
}

function stopApp() {
  if (mqttClient) mqttClient.end(true);
  mqttClient = null;
  clearInterval(chartTimer);
}

// ---------- Realtime via MQTT ----------
function setStatus(text, cls) {
  $("device-status").textContent = text;
  $("device-status").className = "badge " + (cls || "");
}

function connectMqtt(cfg) {
  const tTele = cfg.topic_prefix + "dht/telemetri";
  const tStatus = cfg.topic_prefix + "dht/status";
  mqttClient = mqtt.connect(cfg.mqtt_url, {
    username: cfg.mqtt_user,
    password: cfg.mqtt_pass,
    clientId: "web-" + Math.random().toString(16).slice(2, 8),
    reconnectPeriod: 4000,
  });
  mqttClient.on("connect", () => {
    setStatus("Broker terhubung, menunggu ESP32…");
    mqttClient.subscribe([tTele, tStatus]);
  });
  mqttClient.on("close", () => setStatus("Broker terputus", "offline"));
  mqttClient.on("message", (topic, payload) => {
    const text = payload.toString();
    if (topic === tStatus) {
      setStatus(text === "online" ? "ESP32 online" : "ESP32 offline", text === "online" ? "online" : "offline");
      return;
    }
    let d;
    try { d = JSON.parse(text); } catch { return; }
    if (typeof d.suhu !== "number" || typeof d.kelembapan !== "number") return;
    $("suhu").textContent = d.suhu.toFixed(1);
    $("kelembapan").textContent = d.kelembapan.toFixed(1);
    $("last-update").textContent = "Diperbarui " + new Date().toLocaleTimeString("id-ID");
    setStatus("ESP32 online", "online");
  });
}

// ---------- Grafik dari Supabase ----------
async function loadChart() {
  const since = new Date(Date.now() - 24 * 3600 * 1000).toISOString();
  const { data, error } = await db
    .from("sensor_logs")
    .select("suhu, kelembapan, created_at")
    .gte("created_at", since)
    .order("created_at", { ascending: true })
    .limit(1000);
  if (error) return ($("chart-info").textContent = "Gagal memuat riwayat: " + error.message);
  $("chart-info").textContent = data.length ? data.length + " titik data" : "Belum ada riwayat tersimpan.";
  const labels = data.map((r) => new Date(r.created_at).toLocaleTimeString("en-GB", { hour: "2-digit", minute: "2-digit" }));
  const sets = [
    { label: "Suhu (°C)", data: data.map((r) => r.suhu), borderColor: "#c2410c", yAxisID: "ySuhu" },
    { label: "Kelembapan (%)", data: data.map((r) => r.kelembapan), borderColor: "#1d4ed8", yAxisID: "yLembap" },
  ].map((s) => ({ ...s, pointRadius: 0, borderWidth: 2, tension: 0.3 }));
  if (chart) {
    chart.data.labels = labels;
    chart.data.datasets = sets;
    chart.update();
  } else {
    chart = new Chart($("chart"), {
      type: "line",
      data: { labels, datasets: sets },
      options: {
        animation: false,
        maintainAspectRatio: false,
        interaction: { mode: "index", intersect: false },
        scales: {
          x: { ticks: { maxRotation: 0, autoSkip: true, maxTicksLimit: 6 } },
          ySuhu: { position: "left", grace: "10%", title: { display: true, text: "°C" } },
          yLembap: { position: "right", min: 0, max: 100, title: { display: true, text: "%" }, grid: { drawOnChartArea: false } },
        },
      },
    });
  }
}
