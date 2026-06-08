#include "fsm.h"

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "alarm/alarm.h"
#include "net/wifi_mgr.h"

namespace lapguard {
namespace {
State current_state = State::Boot;
State pre_offline_state = State::Disarmed;
SemaphoreHandle_t state_mutex = nullptr;

void log_state_change(State from, State to) {
  Serial.printf("[FSM] %S -> %S\n", fsm_state_name(from), fsm_state_name(to));
}
}  // namespace

void fsm_init() {
  state_mutex = xSemaphoreCreateMutex();
  current_state = State::Disarmed;
  pre_offline_state = State::Disarmed;
  alarm_set_state(current_state);
  Serial.printf("[FSM] State -> %S\n", fsm_state_name(current_state));
}

State fsm_state() {
  if (state_mutex == nullptr) {
    return current_state;
  }
  xSemaphoreTake(state_mutex, portMAX_DELAY);
  const State state = current_state;
  xSemaphoreGive(state_mutex);
  return state;
}

bool fsm_is_armed() {
  if (state_mutex == nullptr) {
    return current_state == State::Armed ||
           (current_state == State::Offline && pre_offline_state == State::Armed);
  }
  xSemaphoreTake(state_mutex, portMAX_DELAY);
  const bool armed =
    current_state == State::Armed ||
    (current_state == State::Offline && pre_offline_state == State::Armed);
  xSemaphoreGive(state_mutex);
  return armed;
}

const __FlashStringHelper* fsm_state_name(State state) {
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

bool fsm_handle_event(Event event) {
  if (state_mutex != nullptr) {
    xSemaphoreTake(state_mutex, portMAX_DELAY);
  }

  State next_state = current_state;

  switch (current_state) {
    case State::Boot:
      if (event == Event::WifiDown) {
        next_state = State::Offline;
      } else if (event == Event::WifiUp) {
        next_state = State::Disarmed;
      }
      break;
    case State::Disarmed:
      if (event == Event::Arm) {
        next_state = State::Armed;
      } else if (event == Event::WifiDown) {
        next_state = State::Offline;
      }
      break;
    case State::Armed:
      if (event == Event::Motion) {
        next_state = State::Triggered;
      } else if (event == Event::Disarm) {
        next_state = State::Disarmed;
      } else if (event == Event::WifiDown) {
        next_state = State::Offline;
      }
      break;
    case State::Triggered:
      if (event == Event::Silence) {
        next_state = State::Armed;
      } else if (event == Event::Disarm) {
        next_state = State::Disarmed;
      } else if (event == Event::AlarmTimeout) {
        if (wifi_is_connected()) {
          next_state = State::Armed;
        } else {
          next_state = State::Offline;
          pre_offline_state = State::Armed;
        }
      }
      break;
    case State::Offline:
      if (event == Event::WifiUp) {
        next_state = pre_offline_state;
      } else if (event == Event::Motion && pre_offline_state == State::Armed) {
        next_state = State::Triggered;
      }
      break;
  }

  if (next_state == current_state) {
    if (state_mutex != nullptr) {
      xSemaphoreGive(state_mutex);
    }
    return false;
  }

  if (next_state == State::Offline && current_state != State::Offline && current_state != State::Boot) {
    pre_offline_state = current_state;
  }

  log_state_change(current_state, next_state);
  current_state = next_state;
  alarm_set_state(current_state);
  if (state_mutex != nullptr) {
    xSemaphoreGive(state_mutex);
  }
  return true;
}
}  // namespace lapguard
