#include "wifi_mgr.h"

#include <WiFi.h>

#include "config.h"

namespace lapguard {
namespace {
unsigned long last_retry_ms = 0;
bool was_connected = false;
}  // namespace

void wifi_init() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  if (strlen(WIFI_SSID) == 0) {
    Serial.println(F("[WIFI] No SSID configured"));
    return;
  }

  Serial.printf("[WIFI] Connecting to %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  last_retry_ms = millis();
}

void wifi_poll() {
  const bool connected = WiFi.status() == WL_CONNECTED;

  if (connected) {
    if (!was_connected) {
      Serial.printf("[WIFI] Connected, IP: %s, RSSI: %d dBm\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
      was_connected = true;
    }
    return;
  }

  if (was_connected) {
    Serial.println(F("[WIFI] Disconnected"));
    was_connected = false;
  }

  if (millis() - last_retry_ms >= 10000UL) {
    Serial.println(F("[WIFI] Reconnecting..."));
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    last_retry_ms = millis();
  }
}

bool wifi_is_connected() {
  return WiFi.status() == WL_CONNECTED;
}

int wifi_rssi() {
  if (!wifi_is_connected()) {
    return 0;
  }

  return WiFi.RSSI();
}
}  // namespace lapguard