#include "alarm.h"
#include "fsm/fsm.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "config.h"

namespace lapguard {
namespace {
State current_state = State::Boot;
unsigned long last_blink_ms = 0;
bool blink_on = false;
unsigned long triggered_start_ms = 0;
SemaphoreHandle_t alarm_mutex = nullptr;

void apply_outputs(bool green_on, bool red_on, bool buzzer_on) {
  digitalWrite(PIN_LED_GREEN, green_on ? HIGH : LOW);
  digitalWrite(PIN_LED_RED, red_on ? HIGH : LOW);
  digitalWrite(PIN_BUZZER, buzzer_on ? HIGH : LOW);
}
}  // namespace

void alarm_init() {
  alarm_mutex = xSemaphoreCreateMutex();
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  alarm_set_state(State::Disarmed);
  Serial.println(F("[ALARM] Initialized"));
}

void alarm_set_state(State state) {
  if (alarm_mutex != nullptr) {
    xSemaphoreTake(alarm_mutex, portMAX_DELAY);
  }
  current_state = state;
  last_blink_ms = 0;
  blink_on = false;

  switch (current_state) {
    case State::Boot:
      apply_outputs(false, false, false);
      break;
    case State::Disarmed:
      apply_outputs(true, false, false);
      break;
    case State::Armed:
      apply_outputs(true, false, false);
      break;
    case State::Triggered:
      triggered_start_ms = millis();
      apply_outputs(false, true, true);
      break;
    case State::Offline:
      apply_outputs(false, false, false);
      break;
  }
  if (alarm_mutex != nullptr) {
    xSemaphoreGive(alarm_mutex);
  }
}

void alarm_poll() {
  if (alarm_mutex != nullptr) {
    xSemaphoreTake(alarm_mutex, portMAX_DELAY);
  }
  const unsigned long now = millis();
  bool alarm_timed_out = false;

  switch (current_state) {
    case State::Boot:
      apply_outputs(false, false, false);
      break;
    case State::Disarmed:
      apply_outputs(true, false, false);
      break;
    case State::Armed:
      if (now - last_blink_ms >= 500UL) {
        blink_on = !blink_on;
        last_blink_ms = now;
      }
      apply_outputs(blink_on, false, false);
      break;
    case State::Triggered:
      apply_outputs(false, true, true);
      if (now - triggered_start_ms >= ALARM_MAX_DURATION_MS) {
        alarm_timed_out = true;
      }
      break;
    case State::Offline:
      if (now - last_blink_ms >= 250UL) {
        blink_on = !blink_on;
        last_blink_ms = now;
      }
      apply_outputs(blink_on, blink_on, false);
      break;
  }

  if (alarm_mutex != nullptr) {
    xSemaphoreGive(alarm_mutex);
  }
  if (alarm_timed_out) {
    fsm_handle_event(Event::AlarmTimeout);
  }
}
}  // namespace lapguard
