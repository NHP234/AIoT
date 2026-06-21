#include "wifi_mgr.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include <time.h>
#include "config.h"

namespace lapguard {
namespace {
WiFiManager wm;
bool was_connected = false;
bool time_sync_requested = false;
bool time_sync_logged = false;
bool time_sync_warning_logged = false;
unsigned long time_sync_request_ms = 0;
unsigned long last_time_check_ms = 0;

constexpr time_t kMinValidEpoch = 1546300800;  // 2019-01-01
constexpr unsigned long kTimeCheckIntervalMs = 1000UL;
constexpr unsigned long kTimeSyncWarningMs = 15000UL;

void request_time_sync() {
  configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com");
  time_sync_requested = true;
  time_sync_logged = false;
  time_sync_warning_logged = false;
  time_sync_request_ms = millis();
  last_time_check_ms = 0;
  Serial.println("[TIME] NTP sync requested");
}

void poll_time_sync() {
  if (!time_sync_requested || time_sync_logged) {
    return;
  }

  const unsigned long now_ms = millis();
  if (now_ms - last_time_check_ms < kTimeCheckIntervalMs) {
    return;
  }
  last_time_check_ms = now_ms;

  const time_t now = time(nullptr);
  if (now >= kMinValidEpoch) {
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    Serial.printf("[TIME] Current time: %02d:%02d:%02d\n",
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    time_sync_logged = true;
    return;
  }

  if (!time_sync_warning_logged && now_ms - time_sync_request_ms >= kTimeSyncWarningMs) {
    Serial.println("[TIME] NTP still pending; continuing without local clock");
    time_sync_warning_logged = true;
  }
}
}  // namespace

void wifi_init() {
  WiFi.mode(WIFI_STA);
  
  // Kiểm tra nút BOOT (GPIO 0) trong vòng 3 giây sau khi khởi động để xóa WiFi
  pinMode(0, INPUT_PULLUP);
  Serial.println("[WIFI] Giu nut BOOT (nut IO0) trong 3 giay toi de xoa WiFi da luu...");
  bool reset_requested = false;
  for (int i = 0; i < 30; i++) {
    // Nháy nhẹ LED xanh (GPIO 26) báo hiệu đang trong cửa sổ chờ
    pinMode(26, OUTPUT);
    digitalWrite(26, !digitalRead(26));
    
    if (digitalRead(0) == LOW) {
      reset_requested = true;
      break;
    }
    delay(100);
  }
  digitalWrite(26, LOW); // Tắt LED xanh
  
  if (reset_requested) {
    Serial.println("[WIFI] Da phat hien nut BOOT duoc giu! Dang xoa WiFi da luu...");
    wm.resetSettings();
    // Nháy LED đỏ (GPIO 27) báo hiệu đã reset thành công
    pinMode(27, OUTPUT);
    for (int i = 0; i < 6; i++) {
      digitalWrite(27, !digitalRead(27));
      delay(150);
    }
    digitalWrite(27, LOW);
  }

  // Styling: Premium Dark & Neon Blue-Violet Theme for Captive Portal
  wm.setCustomHeadElement(
    "<style>"
    "body{background-color:#0f1016;color:#e2e8f0;font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;margin:0;padding:20px;}"
    "div.content{max-width:400px;margin:40px auto;padding:30px;background:#1e1e24;border-radius:16px;box-shadow:0 10px 40px rgba(0,0,0,0.5);border:1px solid rgba(255,255,255,0.05);text-align:center;}"
    "h1,h2,h3{color:#ffffff;font-weight:600;margin-bottom:20px;letter-spacing:0.5px;}"
    "a{color:#06b6d4;text-decoration:none;font-weight:bold;}"
    "button{background:linear-gradient(135deg,#4f46e5,#06b6d4);color:white;border:none;padding:12px 20px;border-radius:8px;font-weight:bold;cursor:pointer;transition:all 0.3s ease;width:100%;margin-top:10px;box-shadow:0 4px 15px rgba(79,70,229,0.3);}"
    "button:hover{background:linear-gradient(135deg,#6366f1,#0891b2);transform:translateY(-2px);box-shadow:0 6px 20px rgba(79,70,229,0.5);}"
    "input[type='text'],input[type='password'],select{background-color:#121214;border:1px solid #334155;color:white;border-radius:8px;padding:12px;margin-bottom:15px;width:100%;box-sizing:border-box;transition:border-color 0.3s;}"
    "input[type='text']:focus,input[type='password']:focus{border-color:#06b6d4;outline:none;box-shadow:0 0 8px rgba(6, 182, 212, 0.3);}"
    ".q{float:right;font-weight:bold;color:#06b6d4;}"
    ".msg{padding:12px;border-radius:8px;margin-bottom:15px;text-align:center;background-color:rgba(59,130,246,0.1);color:#60a5fa;border:1px solid rgba(59,130,246,0.2);}"
    "</style>"
  );

  // Set config portal timeout (120 seconds) to avoid blocking indefinitely if the user is not present
  wm.setConfigPortalTimeout(120);

  // Set AP name uniquely using the MAC Address
  String ap_name = "LapGuard_" + WiFi.macAddress();
  ap_name.replace(":", "");

  Serial.printf("[WIFI] AutoConnect starting on AP: %s\n", ap_name.c_str());
  
  if (wm.autoConnect(ap_name.c_str())) {
    Serial.println("[WIFI] Connected successfully!");
    was_connected = true;
    request_time_sync();
  } else {
    Serial.println("[WIFI] Config portal timed out. Operating in OFFLINE mode.");
    was_connected = false;
    WiFi.mode(WIFI_STA);
  }
}

void wifi_poll() {
  const bool connected = (WiFi.status() == WL_CONNECTED);

  if (connected) {
    if (!was_connected) {
      Serial.printf("[WIFI] Connected, IP: %s, RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
      was_connected = true;
      request_time_sync();
    }
    poll_time_sync();
  } else {
    if (was_connected) {
      Serial.println(F("[WIFI] Disconnected"));
      was_connected = false;
      time_sync_requested = false;
    }
  }
}

bool wifi_is_connected() {
  return (WiFi.status() == WL_CONNECTED);
}

int wifi_rssi() {
  if (!wifi_is_connected()) {
    return 0;
  }
  return WiFi.RSSI();
}

}  // namespace lapguard
