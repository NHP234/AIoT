#include "auth.h"

#include <Preferences.h>

#include "config.h"

namespace lapguard {
namespace {
Preferences preferences;
String stored_pin = DEFAULT_PIN;
uint8_t fail_count = 0;
unsigned long lockout_until_ms = 0;

bool constant_time_equal(const String& left, const String& right) {
  if (left.length() != right.length()) {
    return false;
  }

  uint8_t diff = 0;
  for (size_t index = 0; index < left.length(); ++index) {
    diff |= static_cast<uint8_t>(left[index] ^ right[index]);
  }
  return diff == 0;
}
}  // namespace

void auth_init() {
  preferences.begin("lapguard", false);
  stored_pin = preferences.getString("pin", DEFAULT_PIN);
  if (!auth_is_valid_pin(stored_pin)) {
    stored_pin = DEFAULT_PIN;
    preferences.putString("pin", stored_pin);
  }

  fail_count = 0;
  lockout_until_ms = 0;
  Serial.println(F("[AUTH] Initialized"));
}

bool auth_is_valid_pin(const String& pin) {
  if (pin.length() < PIN_MIN_LEN || pin.length() > PIN_MAX_LEN) {
    return false;
  }

  for (size_t index = 0; index < pin.length(); ++index) {
    if (!isDigit(pin[index])) {
      return false;
    }
  }

  return true;
}

bool auth_is_locked() {
  return lockout_until_ms != 0 && millis() < lockout_until_ms;
}

unsigned long auth_lockout_remaining_ms() {
  if (!auth_is_locked()) {
    return 0;
  }

  return lockout_until_ms - millis();
}

void auth_record_fail() {
  if (fail_count < AUTH_MAX_FAILS) {
    ++fail_count;
  }

  if (fail_count >= AUTH_MAX_FAILS) {
    lockout_until_ms = millis() + AUTH_LOCKOUT_MS;
    fail_count = 0;
    Serial.println(F("[AUTH] Lockout started"));
  }
}

void auth_reset_failures() {
  fail_count = 0;
  lockout_until_ms = 0;
}

bool auth_verify_pin(const String& pin) {
  if (auth_is_locked()) {
    return false;
  }

  if (!auth_is_valid_pin(pin)) {
    auth_record_fail();
    return false;
  }

  if (constant_time_equal(pin, stored_pin)) {
    auth_reset_failures();
    return true;
  }

  auth_record_fail();
  return false;
}

bool auth_change_pin(const String& old_pin, const String& new_pin) {
  if (!auth_verify_pin(old_pin)) {
    return false;
  }

  if (!auth_is_valid_pin(new_pin)) {
    return false;
  }

  stored_pin = new_pin;
  preferences.putString("pin", stored_pin);
  auth_reset_failures();
  Serial.println(F("[AUTH] PIN updated"));
  return true;
}
}  // namespace lapguard