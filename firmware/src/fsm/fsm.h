#pragma once

#include <Arduino.h>

namespace lapguard {
enum class State : uint8_t {
  Boot,
  Disarmed,
  Armed,
  Triggered,
  Offline,
};

enum class Event : uint8_t {
  Motion,
  Arm,
  Disarm,
  Silence,
  WifiUp,
  WifiDown,
  AlarmTimeout,
};

void fsm_init();
State fsm_state();
bool fsm_is_armed();
const __FlashStringHelper* fsm_state_name(State state);
bool fsm_handle_event(Event event);
}  // namespace lapguard