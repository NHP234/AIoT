#include "wifi_mgr.h"
#include <WiFi.h>
#include <WiFiManager.h>
#include "config.h"

namespace lapguard {
namespace {
WiFiManager wm;
bool was_connected = false;
}  // namespace

void wifi_init() {
  WiFi.mode(WIFI_STA);

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
    }
  } else {
    if (was_connected) {
      Serial.println(F("[WIFI] Disconnected"));
      was_connected = false;
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