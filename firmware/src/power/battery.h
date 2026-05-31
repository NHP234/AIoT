#pragma once

#include <Arduino.h>

namespace lapguard {
void battery_init();
void battery_poll();
uint16_t battery_mv();
uint8_t battery_percent();
bool battery_is_low();
bool battery_is_critical();
}  // namespace lapguard