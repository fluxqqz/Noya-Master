// espnow_sender.ino
#include <Arduino.h>
#include <atomic>
#include <esp_arduino_version.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include "dashboard.h"

typedef struct {
  uint8_t     slave_idx;     // 0..3 (Plant 1..4)
  uint8_t     servo_idx;     // 0 = Mouth 1 (GPIO 5), 1 = Mouth 2 (GPIO 1)
  uint8_t     open_deg;      // Resting / open angle
  uint8_t     close_deg;     // Closed mouth angle
  uint32_t    duration_ms;   // Active mouth movement duration
  uint32_t    rest_ms;       // Rest duration before repeating
  uint32_t    delay_next_ms; // Delay before dispatching next step
  const char* desc;          // Step description for dashboard
} RoutineStep;

uint8_t peers[][6] = {
  {0x02, 0x02, 0x00, 0x00, 0x00, 0x02},
  {0x02, 0x02, 0x00, 0x00, 0x00, 0x03},
  {0x02, 0x02, 0x00, 0x00, 0x00, 0x04},
  {0x02, 0x02, 0x00, 0x00, 0x00, 0x05},
};
int peerCount = sizeof(peers) / sizeof(peers[0]);

typedef struct __attribute__((packed)) {
  uint32_t cmd_id;       // Unique command ID for deduplication
  uint8_t  servo_idx;    // 0 = Mouth 1 (GPIO 5), 1 = Mouth 2 (GPIO 1)
  uint8_t  open_angle;   // Open angle & resting position (0..180 deg)
  uint8_t  close_angle;  // Closed position (0..180 deg)
  uint32_t duration_ms;  // Active animation duration in milliseconds (0 = STOP)
  uint32_t rest_ms;      // Rest duration in milliseconds (relays OFF, rests OPEN)
} AnimatronicCommand;

// ─── CONFIG ──────────────────────────────────────────────────────────────────
const char* WIFI_SSID = "Arthatronic";
const char* WIFI_PASS = "animatronics";
// const char* WIFI_SSID = "Beskem-4g";
// const char* WIFI_PASS = "arthabeskem";
const char* MDNS_HOST = "mcu-master";

const char* AP_SSID   = "mcu-master";
const char* AP_PASS   = "12344321";
const bool  AP_HIDDEN = false;
const int   AP_CHANNEL = 6;
const int   AP_MAX_CONN = 4;

// Custom AP network settings
IPAddress AP_LOCAL_IP(192, 168, 10, 1);
IPAddress AP_GATEWAY(192, 168, 10, 1);
IPAddress AP_SUBNET(255, 255, 255, 0);

// ─── OBJECTS ─────────────────────────────────────────────────────────────────
WebServer server(80);

// ─── WIFI & MDNS ─────────────────────────────────────────────────────────────
void initAP() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY, AP_SUBNET);

  bool apOk = WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONN);
  Serial.printf("[AP] %s, SSID: %s, IP: %s\n",
                apOk ? "Started" : "Failed to start",
                AP_SSID,
                WiFi.softAPIP().toString().c_str());

  // wifiTask owns station retries; keep the AP running independently.
  WiFi.setAutoReconnect(false);

  if (MDNS.begin(MDNS_HOST)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("[mDNS] http://%s.local\n", MDNS_HOST);
  }
}

void wifiTask(void* pvParameters) {
  const uint32_t RETRY_INTERVAL_MS = 15000;
  bool wasConnected = false;

  Serial.println("[WiFi] Connecting to STA in background...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t lastAttempt = millis();

  for (;;) {
    const bool connected = WiFi.status() == WL_CONNECTED;
    const uint32_t now = millis();

    if (connected && !wasConnected) {
      Serial.printf("[WiFi] Connected, IP: %s\n",
                    WiFi.localIP().toString().c_str());
    } else if (!connected && wasConnected) {
      Serial.println("[WiFi] Connection lost; retrying in 15 seconds. AP remains enabled.");
      lastAttempt = now;
    }

    if (!connected && uint32_t(now - lastAttempt) >= RETRY_INTERVAL_MS) {
      Serial.println("[WiFi] Retrying STA connection...");
      WiFi.disconnect(false, false);
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      lastAttempt = millis();
    }

    wasConnected = connected;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

// ─── OTA ─────────────────────────────────────────────────────────────────────
void initOTA() {
  ElegantOTA.begin(&server);

  ElegantOTA.onStart([]() {
    Serial.println("[OTA] Update starting...");
  });
  ElegantOTA.onEnd([](bool success) {
    if (success) Serial.println("[OTA] Done, rebooting.");
    else         Serial.println("[OTA] Failed.");
  });
  ElegantOTA.onProgress([](size_t cur, size_t total) {
    Serial.printf("[OTA] Progress: %d / %d\n", cur, total);
  });

  server.begin();

  Serial.println("[OTA] Ready at:");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("  STA: http://%s/update\n", WiFi.localIP().toString().c_str());
  }
  Serial.printf("  AP:  http://%s/update\n", WiFi.softAPIP().toString().c_str());
}

// ─── RUNTIME STATE & TELEMETRY ────────────────────────────────────────────────
static bool     g_routine_enabled = true;
static bool     g_routine_paused  = false;
static bool     g_restart_routine = false;
static int      g_target_step     = -1; // -1 = sequential, >=0 = jump target
static size_t   g_current_step    = 0;
static char     g_current_desc[64] = "Starting up...";
static bool     g_peer_ack[4]     = { false, false, false, false };
static bool     g_peer_sent[4]    = { false, false, false, false };
static SemaphoreHandle_t g_telemetry_mutex = NULL;

void setRoutineDesc(const char* desc) {
  if (g_telemetry_mutex && xSemaphoreTake(g_telemetry_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    snprintf(g_current_desc, sizeof(g_current_desc), "%s", desc);
    xSemaphoreGive(g_telemetry_mutex);
  } else {
    snprintf(g_current_desc, sizeof(g_current_desc), "%s", desc);
  }
}

struct HistoryItem {
  uint32_t cmd_id;
  uint8_t  slave_idx;
  uint8_t  servo_idx;
  uint8_t  open_angle;
  uint8_t  close_angle;
  uint32_t duration_ms;
  uint32_t rest_ms;
  bool     ack;
  uint32_t timestamp_ms;
};

const size_t HISTORY_MAX = 8;
static HistoryItem g_history[HISTORY_MAX];
static size_t g_history_count = 0;
static size_t g_history_head  = 0;

void recordHistory(const AnimatronicCommand& cmd, uint8_t slave_idx, bool initial_ack) {
  if (!g_telemetry_mutex || xSemaphoreTake(g_telemetry_mutex, pdMS_TO_TICKS(50)) != pdTRUE) return;

  HistoryItem& item = g_history[g_history_head];
  item.cmd_id       = cmd.cmd_id;
  item.slave_idx     = slave_idx;
  item.servo_idx     = cmd.servo_idx;
  item.open_angle    = cmd.open_angle;
  item.close_angle   = cmd.close_angle;
  item.duration_ms   = cmd.duration_ms;
  item.rest_ms       = cmd.rest_ms;
  item.ack           = initial_ack;
  item.timestamp_ms  = millis();

  g_history_head = (g_history_head + 1) % HISTORY_MAX;
  if (g_history_count < HISTORY_MAX) g_history_count++;

  xSemaphoreGive(g_telemetry_mutex);
}

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
void onSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  const uint8_t *mac_addr = info->des_addr;
#else
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
  bool success = (status == ESP_NOW_SEND_SUCCESS);
  Serial.printf("[ESP-NOW] Send to " MACSTR " — %s\n",
                MAC2STR(mac_addr),
                success ? "ACK" : "NO-ACK");

  if (g_telemetry_mutex && xSemaphoreTake(g_telemetry_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    for (int i = 0; i < peerCount; i++) {
      if (memcmp(peers[i], mac_addr, 6) == 0) {
        g_peer_ack[i] = success;
        break;
      }
    }
    for (size_t i = 0; i < g_history_count; i++) {
      size_t idx = (g_history_head + HISTORY_MAX - 1 - i) % HISTORY_MAX;
      if (memcmp(peers[g_history[idx].slave_idx], mac_addr, 6) == 0) {
        g_history[idx].ack = success;
        break;
      }
    }
    xSemaphoreGive(g_telemetry_mutex);
  }
}

static std::atomic<uint32_t> g_next_cmd_id{1};

bool sendMouthCommand(uint8_t slave_idx, uint8_t servo_idx, uint8_t open_deg, uint8_t close_deg, uint32_t duration_ms, uint32_t rest_ms = 60000) {
  if (slave_idx >= peerCount || servo_idx >= 2) return false;

  AnimatronicCommand cmd;
  cmd.cmd_id      = g_next_cmd_id++;
  cmd.servo_idx   = servo_idx;
  cmd.open_angle  = open_deg;
  cmd.close_angle = close_deg;
  cmd.duration_ms = duration_ms;
  cmd.rest_ms     = rest_ms;

  esp_err_t result = esp_now_send(peers[slave_idx], (const uint8_t*)&cmd, sizeof(cmd));
  if (result != ESP_OK) {
    vTaskDelay(pdMS_TO_TICKS(10));
    result = esp_now_send(peers[slave_idx], (const uint8_t*)&cmd, sizeof(cmd));
  }

  // Peer link telemetry: a send was attempted; an immediate failure means no response.
  // onSent() refines g_peer_ack with the real delivery status when the callback fires.
  if (g_telemetry_mutex && xSemaphoreTake(g_telemetry_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    g_peer_sent[slave_idx] = true;
    g_peer_ack[slave_idx]  = (result == ESP_OK);
    xSemaphoreGive(g_telemetry_mutex);
  }

  recordHistory(cmd, slave_idx, result == ESP_OK);

  Serial.printf("[Master] Cmd #%u -> Slave %u, Mouth %u (Open:%u Close:%u Dur:%ums Rest:%ums) - %s\n",
                cmd.cmd_id, slave_idx + 1, servo_idx + 1, open_deg, close_deg, duration_ms, rest_ms,
                result == ESP_OK ? "queued" : "fail");
  return (result == ESP_OK);
}

// ─── CHOREOGRAPHY TIMELINE ───────────────────────────────────────────────────
// Preset 0: Staggered Dialogue (Default)
const RoutineStep ROUTINE_STAGGERED[] = {
  { 0, 0, 50, 100, 60000, 60000,     0, "P1S1" },
  { 2, 0, 50, 100, 60000, 60000, 25000, "P3S1" },

  { 0, 1, 50, 100, 60000, 60000,     0, "P1S2" },
  { 2, 1, 50, 100, 60000, 60000, 25000, "P3S2" },

  { 1, 0, 50, 100, 60000, 60000,     0, "P2S1" },
  { 3, 0, 50, 100, 60000, 60000,     0, "P4S1" },
};
const size_t STAGGERED_STEPS = sizeof(ROUTINE_STAGGERED) / sizeof(ROUTINE_STAGGERED[0]);

// Preset 1: All Plants Simultaneous (All 4 plants, both Mouth 1 & Mouth 2)
const RoutineStep ROUTINE_SIMULTANEOUS[] = {
  { 0, 0, 50, 100, 60000, 60000, 0, "P1M1" },
  { 0, 1, 50, 100, 60000, 60000, 0, "P1M2" },
  { 1, 0, 50, 100, 60000, 60000, 0, "P2M1" },
  { 1, 1, 50, 100, 60000, 60000, 0, "P2M2" },
  { 2, 0, 50, 100, 60000, 60000, 0, "P3M1" },
  { 2, 1, 50, 100, 60000, 60000, 0, "P3M2" },
  { 3, 0, 50, 100, 60000, 60000, 0, "P4M1" },
  { 3, 1, 50, 100, 60000, 60000, 0, "P4M2" },
};
const size_t SIMULTANEOUS_STEPS = sizeof(ROUTINE_SIMULTANEOUS) / sizeof(ROUTINE_SIMULTANEOUS[0]);

static int g_active_preset = 0; // 0 = Staggered, 1 = Simultaneous

const RoutineStep* getActiveRoutine() {
  return (g_active_preset == 1) ? ROUTINE_SIMULTANEOUS : ROUTINE_STAGGERED;
}

size_t getActiveRoutineSteps() {
  return (g_active_preset == 1) ? SIMULTANEOUS_STEPS : STAGGERED_STEPS;
}

void moveSinglePlant(int plantIdx) {
  if (plantIdx < 0 || plantIdx >= peerCount) return;
  sendMouthCommand(plantIdx, 0, 50, 100, 60000, 60000);
  sendMouthCommand(plantIdx, 1, 50, 100, 60000, 60000);
}

void moveSingleServo(int plantIdx, int servoIdx) {
  if (plantIdx < 0 || plantIdx >= peerCount || servoIdx < 0 || servoIdx >= 2) return;
  sendMouthCommand(plantIdx, servoIdx, 50, 100, 60000, 60000);
}

void startAllSlaves() {
  size_t total = getActiveRoutineSteps();
  const RoutineStep* routine = getActiveRoutine();
  for (size_t i = 0; i < total; i++) {
    const RoutineStep& step = routine[i];
    if (step.duration_ms > 0) {
      sendMouthCommand(step.slave_idx, step.servo_idx, step.open_deg, step.close_deg, step.duration_ms, step.rest_ms);
    }
  }
}

void stopAllSlaves() {
  for (int s = 0; s < peerCount; s++) {
    sendMouthCommand(s, 0, 50, 50, 0, 0);
    sendMouthCommand(s, 1, 50, 50, 0, 0);
  }
}

void senderTask(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(3000)); // Allow slaves to settle after boot
  setRoutineDesc("Starting routine...");

  for (;;) {
    if (!g_routine_enabled || g_routine_paused) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    g_restart_routine = false;
    size_t total = getActiveRoutineSteps();
    const RoutineStep* routine = getActiveRoutine();

    // Step-by-step staggered dispatch
    for (size_t i = 0; i < total && g_routine_enabled && !g_routine_paused && !g_restart_routine; i++) {
      g_current_step = i + 1;
      const RoutineStep& step = routine[i];
      setRoutineDesc(step.desc);

      if (step.duration_ms > 0) {
        sendMouthCommand(step.slave_idx, step.servo_idx, step.open_deg, step.close_deg, step.duration_ms, step.rest_ms);
      }

      // Responsive delay slicing (yields every 50ms so C6 single core handles Web/OTA)
      uint32_t remaining = step.delay_next_ms;
      while (remaining > 0 && g_routine_enabled && !g_routine_paused && !g_restart_routine) {
        uint32_t slice = (remaining > 50) ? 50 : remaining;
        vTaskDelay(pdMS_TO_TICKS(slice));
        remaining -= slice;
      }
    }

    // Once all plants are dispatched with their offsets, let them loop autonomously
    if (g_routine_enabled && !g_routine_paused && !g_restart_routine) {
      setRoutineDesc("All plants running (autonomous loop)");
      while (g_routine_enabled && !g_routine_paused && !g_restart_routine) {
        vTaskDelay(pdMS_TO_TICKS(500));
      }
    }
  }
}

// ─── DASHBOARD REST API ───────────────────────────────────────────────────────
void handleRoot() {
  server.send_P(200, "text/html", DASHBOARD_HTML);
}

void handleApiStatus() {
  String json = "{";
  json.reserve(1500);

  json += "\"channel\":" + String(AP_CHANNEL) + ",";

  char step_desc[64] = "";
  bool peer_ack_snap[4] = {false};
  bool peer_sent_snap[4] = {false};

  size_t total_steps = getActiveRoutineSteps();
  const RoutineStep* routine = getActiveRoutine();

  // Routine state
  json += "\"routine\":{";
  json += "\"state\":\"";
  if (!g_routine_enabled) json += "STOPPED";
  else if (g_routine_paused) json += "PAUSED";
  else json += "RUNNING";
  json += "\",";
  json += "\"active_preset\":" + String(g_active_preset) + ",";
  json += "\"current_step\":" + String(g_current_step) + ",";
  json += "\"total_steps\":" + String(total_steps) + ",";

  if (g_telemetry_mutex && xSemaphoreTake(g_telemetry_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    strncpy(step_desc, g_current_desc, sizeof(step_desc) - 1);
    step_desc[sizeof(step_desc) - 1] = '\0';
    for (int i = 0; i < peerCount && i < 4; i++) {
      peer_ack_snap[i] = g_peer_ack[i];
      peer_sent_snap[i] = g_peer_sent[i];
    }
    xSemaphoreGive(g_telemetry_mutex);
  } else {
    strncpy(step_desc, g_current_desc, sizeof(step_desc) - 1);
    step_desc[sizeof(step_desc) - 1] = '\0';
  }

  json += "\"step_desc\":\"" + String(step_desc) + "\"},";

  // All Steps in Routine for Visual Timeline
  json += "\"steps\":[";
  for (size_t i = 0; i < total_steps; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"idx\":" + String(i + 1) + ",";
    json += "\"plant\":" + String(routine[i].slave_idx + 1) + ",";
    json += "\"mouth\":" + String(routine[i].servo_idx + 1) + ",";
    json += "\"dur\":" + String(routine[i].duration_ms) + ",";
    json += "\"desc\":\"" + String(routine[i].desc) + "\"";
    json += "}";
  }
  json += "],";

  // Peer Link Status
  json += "\"peers\":[";
  for (int i = 0; i < peerCount; i++) {
    if (i > 0) json += ",";
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             peers[i][0], peers[i][1], peers[i][2], peers[i][3], peers[i][4], peers[i][5]);
    json += "{";
    json += "\"id\":" + String(i + 1) + ",";
    json += "\"name\":\"Plant " + String(i + 1) + "\",";
    json += "\"mac\":\"" + String(macStr) + "\",";
    json += "\"ack\":" + String(peer_ack_snap[i] ? "true" : "false") + ",";
    json += "\"sent\":" + String(peer_sent_snap[i] ? "true" : "false");
    json += "}";
  }
  json += "],";

  // History ring buffer
  json += "\"history\":[";
  if (g_telemetry_mutex && xSemaphoreTake(g_telemetry_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    uint32_t now = millis();
    for (size_t i = 0; i < g_history_count; i++) {
      size_t idx = (g_history_head + HISTORY_MAX - 1 - i) % HISTORY_MAX;
      const HistoryItem& item = g_history[idx];
      if (i > 0) json += ",";
      json += "{";
      json += "\"cmd_id\":" + String(item.cmd_id) + ",";
      json += "\"slave_idx\":" + String(item.slave_idx) + ",";
      json += "\"servo_idx\":" + String(item.servo_idx) + ",";
      json += "\"open_deg\":" + String(item.open_angle) + ",";
      json += "\"close_deg\":" + String(item.close_angle) + ",";
      json += "\"duration_ms\":" + String(item.duration_ms) + ",";
      json += "\"ack\":" + String(item.ack ? "true" : "false") + ",";
      json += "\"time_ago\":" + String((now >= item.timestamp_ms) ? (now - item.timestamp_ms) / 1000 : 0);
      json += "}";
    }
    xSemaphoreGive(g_telemetry_mutex);
  }
  json += "],";

  json += "\"uptime_sec\":" + String(millis() / 1000);
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleApiRoutine() {
  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "select_preset" || (action == "start" && server.hasArg("preset"))) {
      if (server.hasArg("preset")) {
        int p = server.arg("preset").toInt();
        if (p >= 0 && p <= 1) {
          g_active_preset = p;
        }
      }
    }

    if (action == "start") {
      g_routine_enabled = true;
      g_routine_paused  = false;
      g_restart_routine = true;
      setRoutineDesc("Starting routine...");
    } else if (action == "select_preset") {
      if (g_routine_enabled) {
        g_restart_routine = true;
        setRoutineDesc("Preset changed, restarting...");
      }
    } else if (action == "resume") {
      g_routine_paused  = false;
      setRoutineDesc("Routine resumed");
    } else if (action == "pause") {
      g_routine_paused  = true;
      setRoutineDesc("Routine paused");
    } else if (action == "stop") {
      g_routine_enabled = false;
      g_routine_paused  = false;
      g_restart_routine = false;
      g_current_step    = 0;
      g_target_step     = -1;
      setRoutineDesc("Parking to rest position (50°)...");
      stopAllSlaves();
      vTaskDelay(pdMS_TO_TICKS(1500));
      setRoutineDesc("All plants stopped (resting open)");
    }
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleApiAnimate() {
  int slave = server.arg("slave").toInt();
  int mouth = server.arg("mouth").toInt();
  int open  = constrain(server.arg("open").toInt(), 0, 180);
  int close = constrain(server.arg("close").toInt(), 0, 180);
  uint32_t duration = server.hasArg("duration") ? server.arg("duration").toInt() : 60000;
  uint32_t rest     = server.hasArg("rest") ? server.arg("rest").toInt() : 60000;

  bool ok = sendMouthCommand(slave, mouth, open, close, duration, rest);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(ok ? 200 : 400, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

void handleApiQuick() {
  String action = server.arg("action");
  if (action == "all_talk") {
    g_routine_enabled = true;
    g_routine_paused  = false;
    g_current_step    = 1;
    startAllSlaves();
    setRoutineDesc("All plants running (autonomous loop)");
  } else if (action == "move_plant") {
    int p = server.arg("plant").toInt();
    if (p >= 0 && p < peerCount) {
      moveSinglePlant(p);
      char buf[64];
      snprintf(buf, sizeof(buf), "Plant %d moving (autonomous)", p + 1);
      setRoutineDesc(buf);
    }
  } else if (action == "move_servo") {
    int p = server.arg("plant").toInt();
    int s = server.arg("servo").toInt();
    if (p >= 0 && p < peerCount && s >= 0 && s < 2) {
      moveSingleServo(p, s);
      char buf[64];
      snprintf(buf, sizeof(buf), "Plant %d Mouth %d moving (autonomous)", p + 1, s + 1);
      setRoutineDesc(buf);
    }
  } else if (action == "rest_all") {
    g_routine_enabled = false;
    g_routine_paused  = false;
    g_current_step    = 0;
    setRoutineDesc("Parking to rest position (50°)...");
    stopAllSlaves();
    vTaskDelay(pdMS_TO_TICKS(1500));
    setRoutineDesc("All plants stopped (resting open)");
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"ok\":true}");
}

void initWebRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/routine", HTTP_POST, handleApiRoutine);
  server.on("/api/animate", HTTP_POST, handleApiAnimate);
  server.on("/api/quick", HTTP_POST, handleApiQuick);
}

void setup() {
  Serial.begin(115200);

  g_telemetry_mutex = xSemaphoreCreateMutex();

  initAP();
  initWebRoutes();
  initOTA();

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  esp_now_register_send_cb(onSent);
#else
  esp_now_register_send_cb((esp_now_send_cb_t)onSent);
#endif

  for (int i = 0; i < peerCount; i++) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, peers[i], 6);
    peer.channel = AP_CHANNEL;
    peer.encrypt = false;
    esp_err_t addStatus = esp_now_add_peer(&peer);
    Serial.printf("[ESP-NOW] Peer %d (" MACSTR ") added: %s\n",
                  i + 1, MAC2STR(peers[i]),
                  addStatus == ESP_OK ? "OK" : "FAIL");
  }

  // if (xTaskCreatePinnedToCore(wifiTask, "WiFi manager", 16384, NULL, 1, NULL, 0) != pdPASS) {
  //   Serial.println("[WiFi] ERROR: Could not create WiFi task; STA unavailable. AP remains enabled.");
  // }
  // ESP32-C6 single-core RISC-V: priority 1 shares CPU cooperatively with Arduino loopTask
  if (xTaskCreate(senderTask, "Sender task", 8192, NULL, 1, NULL) != pdPASS) {
    Serial.println("[Sender] ERROR: Could not create sender task!");
  }
}

void loop() {
  server.handleClient();
  ElegantOTA.loop();

  vTaskDelay(pdMS_TO_TICKS(1));
}
