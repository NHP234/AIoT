#pragma once

#include <Arduino.h>

#include "fsm/fsm.h"

namespace lapguard {
void alarm_init();
void alarm_poll();
void alarm_set_state(State state);
}  // namespace lapguard