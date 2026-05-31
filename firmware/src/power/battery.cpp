#include "battery.h"

#include "config.h"
#include "net/telegram_bot.h"

namespace lapguard {
namespace {
constexpr uint32_t kPollIntervalMs = 60000UL;

uint16_t current_mv = 0;
unsigned long last_poll_ms = 0;
bool low_alert_sent = false;

uint16_t read_battery_mv() {
  const uint32_t adc_mv = analogReadMilliVolts(PIN_BATTERY_ADC);
  return static_cast<uint16_t>(adc_mv * 2U);
}

uint8_t mv_to_percent(uint16_t mv) {
  if (mv <= BAT_CRITICAL_MV) {
    return 0;
  }

  if (mv >= 4200U) {
    return 100;
  }

  const long clamped_mv = static_cast<long>(mv) - BAT_CRITICAL_MV;
  const long span = 4200L - BAT_CRITICAL_MV;
  return static_cast<uint8_t>((clamped_mv * 100L) / span);
}

void log_status() {
  Serial.printf("[PWR] Battery: %u mV (%u%%)\n", current_mv, battery_percent());
}
}  // namespace

void battery_init() {
  pinMode(PIN_BATTERY_ADC, INPUT);
  analogSetPinAttenuation(PIN_BATTERY_ADC, ADC_11db);
  current_mv = read_battery_mv();
  last_poll_ms = 0;
  low_alert_sent = false;
  log_status();
}

void battery_poll() {
  const unsigned long now = millis();
  if (now - last_poll_ms < kPollIntervalMs) {
    return;
  }
  last_poll_ms = now;

  current_mv = read_battery_mv();
  log_status();

  if (battery_is_low() && !low_alert_sent) {
    low_alert_sent = true;
    String message = F("CANH BAO PIN YEU\nBattery: ");
    message += String(current_mv);
    message += F(" mV");
    telegram_send_text(CHAT_ID_OWNER, message);
  }
}

uint16_t battery_mv() {
  return current_mv;
}

uint8_t battery_percent() {
  return mv_to_percent(current_mv);
}

bool battery_is_low() {
  return current_mv <= BAT_LOW_MV;
}

bool battery_is_critical() {
  return current_mv <= BAT_CRITICAL_MV;
}
}  // namespace lapguard