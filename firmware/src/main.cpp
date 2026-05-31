#include <Arduino.h>

#include "config.h"
#include "alarm/alarm.h"
#include "auth/auth.h"
#include "fsm/fsm.h"
#include "motion/motion.h"
#include "power/battery.h"
#include "net/telegram_bot.h"
#include "net/wifi_mgr.h"

namespace {
unsigned long last_heartbeat_ms = 0;
bool last_wifi_connected = false;

void print_boot_banner() {
  Serial.println();
  Serial.println(F("[BOOT] LapGuard firmware skeleton"));
  Serial.printf("[BOOT] Device: %s\n", DEVICE_NAME);
  Serial.printf("[BOOT] WiFi SSID: %s\n", WIFI_SSID);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(500);
  print_boot_banner();

  lapguard::alarm_init();
  lapguard::auth_init();
  lapguard::fsm_init();
  lapguard::motion_init();
  lapguard::battery_init();
  lapguard::wifi_init();
  lapguard::telegram_init();

  last_wifi_connected = lapguard::wifi_is_connected();
  lapguard::fsm_handle_event(last_wifi_connected ? lapguard::Event::WifiUp : lapguard::Event::WifiDown);

  Serial.println(F("[BOOT] Ready for module development"));
}

void loop() {
  lapguard::wifi_poll();
  lapguard::alarm_poll();
  lapguard::battery_poll();

  const bool wifi_connected = lapguard::wifi_is_connected();
  if (wifi_connected != last_wifi_connected) {
    lapguard::fsm_handle_event(wifi_connected ? lapguard::Event::WifiUp : lapguard::Event::WifiDown);
    last_wifi_connected = wifi_connected;
  }

  if (wifi_connected) {
    lapguard::telegram_poll();
  }

  if (lapguard::fsm_state() == lapguard::State::Armed && lapguard::motion_poll()) {
    lapguard::fsm_handle_event(lapguard::Event::Motion);
    lapguard::telegram_send_alert(lapguard::motion_average());
  }

  const unsigned long now = millis();
  if (now - last_heartbeat_ms >= 5000UL) {
    Serial.println(F("[LOOP] firmware alive"));
    last_heartbeat_ms = now;
  }
  delay(10);
}