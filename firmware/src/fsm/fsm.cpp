#include "fsm.h"

#include "alarm/alarm.h"

namespace lapguard {
namespace {
State current_state = State::Boot;

void log_state_change(State from, State to) {
  Serial.printf("[FSM] %S -> %S\n", fsm_state_name(from), fsm_state_name(to));
}
}  // namespace

void fsm_init() {
  current_state = State::Disarmed;
  alarm_set_state(current_state);
  Serial.printf("[FSM] State -> %S\n", fsm_state_name(current_state));
}

State fsm_state() {
  return current_state;
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
      }
      break;
    case State::Offline:
      if (event == Event::WifiUp) {
        next_state = State::Disarmed;
      }
      break;
  }

  if (next_state == current_state) {
    return false;
  }

  log_state_change(current_state, next_state);
  current_state = next_state;
  alarm_set_state(current_state);
  return true;
}
}  // namespace lapguard