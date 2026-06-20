#include "firebase_mgr.h"
#include <Firebase_ESP_Client.h>
#include <WiFi.h>
#include "config.h"
#include "fsm/fsm.h"
#include "power/battery.h"
#include "net/wifi_mgr.h"

namespace lapguard {
namespace {
FirebaseData fbdo_stream;
FirebaseData fbdo_write;
FirebaseAuth auth;
FirebaseConfig config;

String device_mac = "";
String device_path = "";
String command_path = "";

bool firebase_connected = false;
unsigned long last_stream_check_ms = 0;
constexpr unsigned long kStreamCheckIntervalMs = 5000UL;
constexpr unsigned long kWriteRetryIntervalMs = 500UL;
constexpr size_t kPendingAlertCapacity = 8;

struct PendingAlert {
  float delta_g = 0.0f;
};

PendingAlert pending_alerts[kPendingAlertCapacity];
size_t pending_alert_head = 0;
size_t pending_alert_count = 0;
String pending_status;
bool status_pending = false;
unsigned long last_write_attempt_ms = 0;

bool firebase_ready_for_writes() {
  return firebase_connected && WiFi.isConnected();
}

void enqueue_alert(float delta_g) {
  if (pending_alert_count == kPendingAlertCapacity) {
    pending_alert_head = (pending_alert_head + 1) % kPendingAlertCapacity;
    --pending_alert_count;
    Serial.println("[FIREBASE] Alert queue full, dropping oldest alert");
  }

  const size_t tail = (pending_alert_head + pending_alert_count) % kPendingAlertCapacity;
  pending_alerts[tail].delta_g = delta_g;
  ++pending_alert_count;
  Serial.printf("[FIREBASE] Alert queued (pending=%u)\n", static_cast<unsigned>(pending_alert_count));
}

bool write_status_now(const String& status_str) {
  if (!firebase_ready_for_writes()) return false;

  String path = device_path + "/status";
  if (Firebase.RTDB.setString(&fbdo_write, path.c_str(), status_str.c_str())) {
    Serial.printf("[FIREBASE] Status updated to: %s\n", status_str.c_str());
    return true;
  }

  Serial.printf("[FIREBASE] Status update failed: %s\n", fbdo_write.errorReason().c_str());
  return false;
}

bool write_alert_now(float delta_g) {
  if (!firebase_ready_for_writes()) return false;

  FirebaseJson json;
  json.add("device_id", device_mac);
  
  FirebaseJson server_ts;
  server_ts.add(".sv", "timestamp");
  json.add("timestamp", server_ts);
  
  json.add("event_type", "MOTION_ALERT");
  
  char detail_buf[64];
  snprintf(detail_buf, sizeof(detail_buf), "Phat hien rung lac manh (delta = %.2fg)", delta_g);
  json.add("detail", detail_buf);
  json.add("resolved", false);

  if (Firebase.RTDB.pushJSON(&fbdo_write, "/logs", &json)) {
    Serial.println("[FIREBASE] Alert log pushed successfully");
    return true;
  }

  Serial.printf("[FIREBASE] Alert log push failed: %s\n", fbdo_write.errorReason().c_str());
  return false;
}

void process_pending_writes() {
  if (!firebase_ready_for_writes()) return;

  const unsigned long now = millis();
  if (now - last_write_attempt_ms < kWriteRetryIntervalMs) {
    return;
  }
  last_write_attempt_ms = now;

  if (status_pending && write_status_now(pending_status)) {
    status_pending = false;
  }

  if (pending_alert_count > 0) {
    const PendingAlert alert = pending_alerts[pending_alert_head];
    if (write_alert_now(alert.delta_g)) {
      pending_alert_head = (pending_alert_head + 1) % kPendingAlertCapacity;
      --pending_alert_count;
    }
  }
}
}  // namespace

void firebase_init() {
  // Get MAC Address and format as XX:XX:XX:XX:XX:XX
  device_mac = WiFi.macAddress();
  device_path = "/devices/" + device_mac;
  command_path = device_path + "/command";

  Serial.printf("[FIREBASE] Device MAC: %s\n", device_mac.c_str());
  Serial.printf("[FIREBASE] Connecting to host: %s\n", FIREBASE_HOST);

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  if (!WiFi.isConnected()) {
    Serial.println("[FIREBASE] WiFi offline, stream start deferred");
    firebase_connected = false;
    return;
  }

  if (Firebase.RTDB.beginStream(&fbdo_stream, command_path.c_str())) {
    Serial.println("[FIREBASE] Stream started successfully");
    firebase_connected = true;
  } else {
    Serial.printf("[FIREBASE] Stream begin error: %s\n", fbdo_stream.errorReason().c_str());
    firebase_connected = false;
  }
}

void firebase_poll() {
  if (!WiFi.isConnected()) {
    firebase_connected = false;
    return;
  }

  const unsigned long now = millis();

  // Periodically check/restart stream if it failed or timed out
  if (now - last_stream_check_ms >= kStreamCheckIntervalMs) {
    last_stream_check_ms = now;

    if (!firebase_connected || !fbdo_stream.httpConnected()) {
      Serial.println("[FIREBASE] Reconnecting stream...");
      Firebase.RTDB.endStream(&fbdo_stream);
      if (Firebase.RTDB.beginStream(&fbdo_stream, command_path.c_str())) {
        Serial.println("[FIREBASE] Stream reconnected");
        firebase_connected = true;
      } else {
        Serial.printf("[FIREBASE] Stream reconnect failed: %s\n", fbdo_stream.errorReason().c_str());
        firebase_connected = false;
      }
    }
  }

  // Stream command logic
  if (firebase_connected) {
    if (!Firebase.RTDB.readStream(&fbdo_stream)) {
      Serial.printf("[FIREBASE] Stream read error: %s\n", fbdo_stream.errorReason().c_str());
      firebase_connected = false;
    } else {
      if (fbdo_stream.streamTimeout()) {
        // Just stream timeout, normal keep-alive
      }

      if (fbdo_stream.streamAvailable()) {
        if (fbdo_stream.dataType() == "string") {
          String cmd = fbdo_stream.stringData();
          Serial.printf("[FIREBASE] Command received: %s\n", cmd.c_str());
          
          if (cmd == "ARM") {
            fsm_handle_event(Event::Arm);
            Firebase.RTDB.setString(&fbdo_write, command_path.c_str(), "NONE");
          } else if (cmd == "DISARM") {
            fsm_handle_event(Event::Disarm);
            Firebase.RTDB.setString(&fbdo_write, command_path.c_str(), "NONE");
          } else if (cmd == "SILENCE") {
            fsm_handle_event(Event::Silence);
            Firebase.RTDB.setString(&fbdo_write, command_path.c_str(), "NONE");
          }
        }
      }
    }
  }

  process_pending_writes();

  // Periodically update battery & rssi (every 10 seconds)
  static unsigned long last_status_update_ms = 0;
  static bool firebase_low_alert_sent = false;

  if (now - last_status_update_ms >= 10000UL) {
    last_status_update_ms = now;
    if (firebase_connected && WiFi.isConnected()) {
      firebase_update_battery_and_rssi(battery_percent(), wifi_rssi());

      // Push low battery log if it triggers
      if (battery_is_low()) {
        if (!firebase_low_alert_sent) {
          firebase_low_alert_sent = true;
          
          FirebaseJson json;
          json.add("device_id", device_mac);
          FirebaseJson server_ts;
          server_ts.add(".sv", "timestamp");
          json.add("timestamp", server_ts);
          json.add("event_type", "BATTERY_LOW");
          
          char detail_buf[64];
          snprintf(detail_buf, sizeof(detail_buf), "Pin yeu, xin sac lai (%u mV)", battery_mv());
          json.add("detail", detail_buf);
          json.add("resolved", false);

          if (Firebase.RTDB.pushJSON(&fbdo_write, "/logs", &json)) {
            Serial.println("[FIREBASE] Low battery log pushed successfully");
          } else {
            Serial.printf("[FIREBASE] Low battery log push failed: %s\n", fbdo_write.errorReason().c_str());
          }
        }
      } else {
        firebase_low_alert_sent = false;
      }
    }
  }
}

void firebase_send_alert(float delta_g) {
  enqueue_alert(delta_g);
}

void firebase_update_status(const String& status_str) {
  pending_status = status_str;
  status_pending = true;
}

void firebase_update_battery_and_rssi(uint8_t battery, int rssi) {
  if (!WiFi.isConnected()) return;

  FirebaseJson json;
  json.add("battery_percent", battery);
  json.add("wifi_rssi", rssi);
  
  FirebaseJson server_ts;
  server_ts.add(".sv", "timestamp");
  json.add("last_seen", server_ts);

  if (Firebase.RTDB.updateNode(&fbdo_write, device_path.c_str(), &json)) {
    Serial.printf("[FIREBASE] Battery & RSSI updated (Bat: %u%%, RSSI: %d dBm)\n", battery, rssi);
  } else {
    Serial.printf("[FIREBASE] Battery & RSSI update failed: %s\n", fbdo_write.errorReason().c_str());
  }
}

}  // namespace lapguard
