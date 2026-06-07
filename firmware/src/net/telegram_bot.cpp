#include "telegram_bot.h"

#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <vector>

#include "config.h"
#include "auth/auth.h"
#include "fsm/fsm.h"
#include "motion/motion.h"
#include "power/battery.h"
#include "wifi_mgr.h"

namespace lapguard {
namespace {
WiFiClientSecure secure_client;
UniversalTelegramBot bot(BOT_TOKEN, secure_client);
unsigned long last_poll_ms = 0;

struct OfflineEvent {
  unsigned long event_ms;
  float delta_g;
};

constexpr size_t kMaxOfflineEvents = 20;
std::vector<OfflineEvent> offline_queue;

String state_to_string(State state) {
  switch (state) {
    case State::Boot:
      return F("BOOT");
    case State::Disarmed:
      return F("DISARMED");
    case State::Armed:
      return F("ARMED");
    case State::Triggered:
      return F("TRIGGERED");
    case State::Offline:
      return F("OFFLINE");
  }

  return F("UNKNOWN");
}

String build_help_message() {
  String message;
  message.reserve(320);
  message += F("LapGuard commands:\n");
  message += F("/arm <PIN>\n");
  message += F("/disarm <PIN>\n");
  message += F("/silence <PIN>\n");
  message += F("/setpin <OLD> <NEW>\n");
  message += F("/reboot <PIN>\n");
  message += F("/status\n");
  message += F("/help");
  return message;
}

String build_status_message() {
  String message;
  message.reserve(320);
  message += F("LapGuard status\n");
  message += F("State: ");
  message += state_to_string(fsm_state());
  message += F("\nWiFi: ");
  message += wifi_is_connected() ? F("connected") : F("offline");
  message += F("\nRSSI: ");
  message += String(wifi_rssi());
  message += F(" dBm\nMotion avg: ");
  message += String(motion_average(), 3);
  message += F(" g\nMotion count: ");
  message += String(motion_consecutive_count());
  message += F("\nBattery: ");
  message += String(battery_mv());
  message += F(" mV (");
  message += String(battery_percent());
  message += F("%)");
  message += F("\nBattery low: ");
  message += battery_is_low() ? F("yes") : F("no");
  message += F("\nAuth lock: ");
  message += auth_is_locked() ? F("yes") : F("no");
  if (auth_is_locked()) {
    message += F(" (remaining ");
    message += String(auth_lockout_remaining_ms() / 1000UL);
    message += F("s)");
  }
  return message;
}

String extract_argument(const String& text) {
  const int space_index = text.indexOf(' ');
  if (space_index < 0 || space_index + 1 >= static_cast<int>(text.length())) {
    return String();
  }

  String argument = text.substring(space_index + 1);
  argument.trim();
  return argument;
}

bool split_two_arguments(const String& text, String& first, String& second) {
  const int first_space = text.indexOf(' ');
  if (first_space < 0 || first_space + 1 >= static_cast<int>(text.length())) {
    return false;
  }

  String remainder = text.substring(first_space + 1);
  remainder.trim();
  const int second_space = remainder.indexOf(' ');
  if (second_space < 0) {
    return false;
  }

  first = remainder.substring(0, second_space);
  second = remainder.substring(second_space + 1);
  first.trim();
  second.trim();
  return first.length() > 0 && second.length() > 0;
}

void handle_message(const String& chat_id, const String& text) {
  if (chat_id != CHAT_ID_OWNER) {
    Serial.printf("[TG] Ignored chat_id=%s\n", chat_id.c_str());
    return;
  }

  Serial.printf("[TG] cmd=%s\n", text.c_str());

  if (text.startsWith(F("/start")) || text.startsWith(F("/help"))) {
    telegram_send_text(chat_id, build_help_message());
    return;
  }

  if (text.startsWith(F("/status"))) {
    telegram_send_text(chat_id, build_status_message());
    return;
  }

  if (text.startsWith(F("/arm"))) {
    const String pin = extract_argument(text);
    if (pin.length() == 0) {
      telegram_send_text(chat_id, F("Usage: /arm <PIN>"));
      return;
    }

    if (!auth_verify_pin(pin)) {
      telegram_send_text(chat_id, F("PIN sai hoặc đang bị khóa."));
      return;
    }

    fsm_handle_event(Event::Arm);
    telegram_send_text(chat_id, F("ARMED"));
    return;
  }

  if (text.startsWith(F("/disarm"))) {
    const String pin = extract_argument(text);
    if (pin.length() == 0) {
      telegram_send_text(chat_id, F("Usage: /disarm <PIN>"));
      return;
    }

    if (!auth_verify_pin(pin)) {
      telegram_send_text(chat_id, F("PIN sai hoặc đang bị khóa."));
      return;
    }

    fsm_handle_event(Event::Disarm);
    telegram_send_text(chat_id, F("DISARMED"));
    return;
  }

  if (text.startsWith(F("/silence"))) {
    const String pin = extract_argument(text);
    if (pin.length() == 0) {
      telegram_send_text(chat_id, F("Usage: /silence <PIN>"));
      return;
    }

    if (!auth_verify_pin(pin)) {
      telegram_send_text(chat_id, F("PIN sai hoặc đang bị khóa."));
      return;
    }

    fsm_handle_event(Event::Silence);
    telegram_send_text(chat_id, F("SILENCED"));
    return;
  }

  if (text.startsWith(F("/setpin"))) {
    String old_pin;
    String new_pin;
    if (!split_two_arguments(text, old_pin, new_pin)) {
      telegram_send_text(chat_id, F("Usage: /setpin <OLD> <NEW>"));
      return;
    }

    if (!auth_change_pin(old_pin, new_pin)) {
      telegram_send_text(chat_id, F("Doi PIN that bai."));
      return;
    }

    telegram_send_text(chat_id, F("PIN da duoc cap nhat"));
    return;
  }

  if (text.startsWith(F("/reboot"))) {
    const String pin = extract_argument(text);
    if (pin.length() == 0) {
      telegram_send_text(chat_id, F("Usage: /reboot <PIN>"));
      return;
    }

    if (!auth_verify_pin(pin)) {
      telegram_send_text(chat_id, F("PIN sai hoặc đang bị khóa."));
      return;
    }

    telegram_send_text(chat_id, F("Rebooting..."));
    delay(250);
    ESP.restart();
    return;
  }

  telegram_send_text(chat_id, build_help_message());
}
}  // namespace

void telegram_init() {
  secure_client.setInsecure();
  last_poll_ms = 0;
  Serial.println(F("[TG] Initialized"));
}

bool telegram_send_text(const String& chat_id, const String& text) {
  if (!wifi_is_connected()) {
    Serial.println(F("[TG] Send skipped: WiFi offline"));
    return false;
  }

  const unsigned long started_ms = millis();
  const bool sent = bot.sendMessage(chat_id, text, "");
  Serial.printf("[TG] sendMessage %s in %lu ms\n",
                sent ? "OK" : "FAILED", millis() - started_ms);
  return sent;
}

bool telegram_send_alert(float delta_g) {
  if (!wifi_is_connected()) {
    if (offline_queue.size() < kMaxOfflineEvents) {
      offline_queue.push_back({millis(), delta_g});
      Serial.printf("[TG] WiFi down, queued offline event: delta=%.3f g (queue size=%d)\n", delta_g, (int)offline_queue.size());
    } else {
      Serial.println(F("[TG] Offline queue full, event discarded"));
    }
    return false;
  }

  String message;
  message.reserve(256);
  message += F("CANH BAO LAPGUARD!\n");
  message += F("Motion delta: ");
  message += String(delta_g, 3);
  message += F(" g\nState: ");
  message += state_to_string(fsm_state());
  message += F("\nRSSI: ");
  message += String(wifi_rssi());
  message += F(" dBm");
  Serial.printf("[TG] Sending motion alert: delta=%.3f g\n", delta_g);
  return telegram_send_text(CHAT_ID_OWNER, message);
}

bool telegram_send_status() {
  return telegram_send_text(CHAT_ID_OWNER, build_status_message());
}

void telegram_poll() {
  if (!wifi_is_connected()) {
    return;
  }

  // Flush offline queue if not empty
  if (!offline_queue.empty()) {
    String message;
    message.reserve(512);
    message += F("[WiFi tro lai]\nTrong thoi gian offline da xay ra:\n");
    for (const auto& event : offline_queue) {
      const unsigned long elapsed_sec = (millis() - event.event_ms) / 1000UL;
      const unsigned long min = elapsed_sec / 60UL;
      const unsigned long sec = elapsed_sec % 60UL;
      message += F("- Cach day ");
      if (min > 0) {
        message += String(min);
        message += F(" phut ");
      }
      message += String(sec);
      message += F(" giay: MOTION delta=");
      message += String(event.delta_g, 3);
      message += F(" g\n");
    }
    message += F("\nState hien tai: ");
    message += state_to_string(fsm_state());

    Serial.println(F("[TG] Flushing offline queue..."));
    if (telegram_send_text(CHAT_ID_OWNER, message)) {
      offline_queue.clear();
      Serial.println(F("[TG] Offline queue flushed successfully"));
    } else {
      Serial.println(F("[TG] Failed to flush offline queue, will retry"));
      return; // Retry next time
    }
  }

  if (millis() - last_poll_ms < 1000UL) {
    return;
  }
  last_poll_ms = millis();

  int num_new_messages = bot.getUpdates(bot.last_message_received + 1);
  while (num_new_messages > 0) {
    for (int index = 0; index < num_new_messages; ++index) {
      handle_message(bot.messages[index].chat_id, bot.messages[index].text);
    }
    num_new_messages = bot.getUpdates(bot.last_message_received + 1);
  }
}
}  // namespace lapguard
