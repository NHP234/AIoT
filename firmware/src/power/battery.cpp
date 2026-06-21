#include "battery.h"

#include "config.h"

namespace lapguard {
namespace {
constexpr uint32_t kPollIntervalMs = 60000UL;
constexpr uint16_t kBatteryValidMinMv = 2500;
constexpr uint16_t kBatteryValidMaxMv = 5000;

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

bool reading_is_valid(uint16_t mv) {
  return mv >= kBatteryValidMinMv && mv <= kBatteryValidMaxMv;
}

void log_status() {
  if (!reading_is_valid(current_mv)) {
    Serial.printf("[PWR] Battery sense invalid: %u mV (check GPIO34 divider)\n", current_mv);
    return;
  }

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
    Serial.printf("[PWR] Battery warning: low voltage (%u mV)\n", current_mv);
  } else if (!battery_is_low()) {
    low_alert_sent = false;
  }
}

uint16_t battery_mv() {
  return current_mv;
}

uint8_t battery_percent() {
  if (!reading_is_valid(current_mv)) {
    return 0;
  }

  return mv_to_percent(current_mv);
}

bool battery_is_low() {
  return reading_is_valid(current_mv) && current_mv <= BAT_LOW_MV;
}

bool battery_is_critical() {
  return reading_is_valid(current_mv) && current_mv <= BAT_CRITICAL_MV;
}
}  // namespace lapguard
