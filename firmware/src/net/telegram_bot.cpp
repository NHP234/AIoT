#include "telegram_bot.h"

#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include <cstring>
#include <vector>

#include "config.h"
#include "auth/auth.h"
#include "fsm/fsm.h"
#include "motion/motion.h"
#include "power/battery.h"
#include "wifi_mgr.h"

namespace lapguard {
namespace {
WiFiClientSecure send_client;
WiFiClientSecure poll_client;
UniversalTelegramBot send_bot(BOT_TOKEN, send_client);
UniversalTelegramBot poll_bot(BOT_TOKEN, poll_client);
unsigned long last_poll_ms = 0;
unsigned long last_slow_poll_log_ms = 0;
QueueHandle_t outbound_queue = nullptr;
TaskHandle_t send_task_handle = nullptr;
TaskHandle_t poll_task_handle = nullptr;

enum class OutboundType : uint8_t {
  Text,
  MotionAlert,
};

struct OutboundMessage {
  OutboundType type;
  char chat_id[24];
  char text[600];
  float delta_g;
  unsigned long queued_ms;
};

struct OfflineEvent {
  unsigned long event_ms;
  float delta_g;
};

constexpr size_t kMaxOfflineEvents = 20;
std::vector<OfflineEvent> offline_queue;

bool copy_to_buffer(const String& source, char* destination, size_t capacity) {
  if (source.length() >= capacity) {
    return false;
  }

  std::strncpy(destination, source.c_str(), capacity);
  destination[capacity - 1] = '\0';
  return true;
}

bool enqueue_message(const OutboundMessage& message) {
  if (outbound_queue == nullptr) {
    Serial.println(F("[TG] Outbound queue is not initialized"));
    return false;
  }

  if (xQueueSend(outbound_queue, &message, 0) != pdTRUE) {
    Serial.println(F("[TG] Outbound queue full, message dropped"));
    return false;
  }
  return true;
}

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

bool send_text_now(const String& chat_id, const String& text,
                   unsigned long queued_ms) {
  if (!wifi_is_connected()) {
    return false;
  }

  send_bot.waitForResponse = TELEGRAM_SEND_RESPONSE_MS;
  const unsigned long started_ms = millis();
  const bool sent = send_bot.sendMessage(chat_id, text, "");
  const unsigned long finished_ms = millis();
  Serial.printf("[TG] sendMessage %s: queue=%lu ms, network=%lu ms, total=%lu ms\n",
                sent ? "OK" : "FAILED",
                started_ms - queued_ms,
                finished_ms - started_ms,
                finished_ms - queued_ms);
  return sent;
}

String build_alert_message(float delta_g) {
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
  return message;
}

void store_offline_alert(float delta_g, unsigned long event_ms) {
  if (offline_queue.size() < kMaxOfflineEvents) {
    offline_queue.push_back({event_ms, delta_g});
    Serial.printf("[TG] WiFi down, queued offline event: delta=%.3f g (queue size=%d)\n",
                  delta_g, static_cast<int>(offline_queue.size()));
  } else {
    Serial.println(F("[TG] Offline queue full, event discarded"));
  }
}

void process_outbound_message(const OutboundMessage& message) {
  if (message.type == OutboundType::MotionAlert) {
    if (!wifi_is_connected()) {
      store_offline_alert(message.delta_g, message.queued_ms);
      return;
    }

    Serial.printf("[TG] Sending motion alert: delta=%.3f g\n", message.delta_g);
    if (!send_text_now(CHAT_ID_OWNER, build_alert_message(message.delta_g),
                       message.queued_ms)) {
      store_offline_alert(message.delta_g, message.queued_ms);
    }
    return;
  }

  if (!wifi_is_connected()) {
    Serial.println(F("[TG] Text message dropped: WiFi offline"));
    return;
  }
  send_text_now(message.chat_id, message.text, message.queued_ms);
}

bool flush_offline_queue() {
  if (offline_queue.empty() || !wifi_is_connected()) {
    return true;
  }

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
  if (!send_text_now(CHAT_ID_OWNER, message, millis())) {
    Serial.println(F("[TG] Failed to flush offline queue, will retry"));
    return false;
  }

  offline_queue.clear();
  Serial.println(F("[TG] Offline queue flushed successfully"));
  return true;
}

void poll_updates() {
  if (millis() - last_poll_ms < TELEGRAM_POLL_INTERVAL_MS) {
    return;
  }
  last_poll_ms = millis();

  poll_bot.waitForResponse = TELEGRAM_POLL_RESPONSE_MS;
  const unsigned long started_ms = millis();
  int num_new_messages = poll_bot.getUpdates(poll_bot.last_message_received + 1);
  const unsigned long elapsed_ms = millis() - started_ms;
  if (elapsed_ms > TELEGRAM_POLL_RESPONSE_MS + 1000UL &&
      millis() - last_slow_poll_log_ms >= 30000UL) {
    Serial.printf("[TG] getUpdates slow: %lu ms\n", elapsed_ms);
    last_slow_poll_log_ms = millis();
  }

  while (num_new_messages > 0) {
    for (int index = 0; index < num_new_messages; ++index) {
      handle_message(poll_bot.messages[index].chat_id,
                     poll_bot.messages[index].text);
    }

    if (uxQueueMessagesWaiting(outbound_queue) > 0) {
      break;
    }

    poll_bot.waitForResponse = TELEGRAM_POLL_RESPONSE_MS;
    num_new_messages =
      poll_bot.getUpdates(poll_bot.last_message_received + 1);
  }
}

void telegram_send_task(void*) {
  OutboundMessage message{};
  for (;;) {
    if (xQueueReceive(outbound_queue, &message, pdMS_TO_TICKS(50)) == pdTRUE) {
      process_outbound_message(message);
      continue;
    }

    if (wifi_is_connected() && !flush_offline_queue()) {
      vTaskDelay(pdMS_TO_TICKS(250));
    }
  }
}

void telegram_poll_task(void*) {
  for (;;) {
    if (wifi_is_connected()) {
      poll_updates();
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
}  // namespace

void telegram_init() {
  send_client.setInsecure();
  poll_client.setInsecure();
  send_client.setTimeout(TELEGRAM_SEND_RESPONSE_MS);
  poll_client.setTimeout(TELEGRAM_POLL_RESPONSE_MS);
  send_bot.waitForResponse = TELEGRAM_SEND_RESPONSE_MS;
  poll_bot.waitForResponse = TELEGRAM_POLL_RESPONSE_MS;
  last_poll_ms = 0;
  last_slow_poll_log_ms = 0;
  outbound_queue = xQueueCreate(TELEGRAM_QUEUE_LENGTH, sizeof(OutboundMessage));
  if (outbound_queue == nullptr) {
    Serial.println(F("[TG] Failed to create outbound queue"));
    return;
  }

  const BaseType_t send_task_created = xTaskCreatePinnedToCore(
    telegram_send_task,
    "TelegramSend",
    TELEGRAM_SEND_TASK_STACK,
    nullptr,
    3,
    &send_task_handle,
    0);
  const BaseType_t poll_task_created = xTaskCreatePinnedToCore(
    telegram_poll_task,
    "TelegramPoll",
    TELEGRAM_POLL_TASK_STACK,
    nullptr,
    1,
    &poll_task_handle,
    0);
  if (send_task_created != pdPASS || poll_task_created != pdPASS) {
    Serial.println(F("[TG] Failed to create Telegram tasks"));
    if (send_task_handle != nullptr) {
      vTaskDelete(send_task_handle);
      send_task_handle = nullptr;
    }
    if (poll_task_handle != nullptr) {
      vTaskDelete(poll_task_handle);
      poll_task_handle = nullptr;
    }
    vQueueDelete(outbound_queue);
    outbound_queue = nullptr;
    return;
  }
  Serial.println(F("[TG] Initialized async send/poll tasks on core 0"));
}

bool telegram_send_text(const String& chat_id, const String& text) {
  OutboundMessage message{};
  message.type = OutboundType::Text;
  message.queued_ms = millis();
  if (!copy_to_buffer(chat_id, message.chat_id, sizeof(message.chat_id)) ||
      !copy_to_buffer(text, message.text, sizeof(message.text))) {
    Serial.println(F("[TG] Text message exceeds queue buffer"));
    return false;
  }

  return enqueue_message(message);
}

bool telegram_send_alert(float delta_g) {
  OutboundMessage message{};
  message.type = OutboundType::MotionAlert;
  message.delta_g = delta_g;
  message.queued_ms = millis();
  return enqueue_message(message);
}

bool telegram_send_status() {
  return telegram_send_text(CHAT_ID_OWNER, build_status_message());
}
}  // namespace lapguard
