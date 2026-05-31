#pragma once

#include <Arduino.h>

namespace lapguard {
void auth_init();
bool auth_is_valid_pin(const String& pin);
bool auth_is_locked();
unsigned long auth_lockout_remaining_ms();
bool auth_verify_pin(const String& pin);
bool auth_change_pin(const String& old_pin, const String& new_pin);
void auth_record_fail();
void auth_reset_failures();
}  // namespace lapguard