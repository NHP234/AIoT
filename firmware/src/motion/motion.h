#pragma once

#include <Arduino.h>

namespace lapguard {
void motion_init();
void motion_reset();
bool motion_poll();
bool motion_sensor_ready();
bool motion_push_sample(float delta_g);
bool motion_triggered();
float motion_average();
uint8_t motion_consecutive_count();
}  // namespace lapguard