#include "auth.h"

#include <Preferences.h>
#include <esp_system.h>
#include <mbedtls/sha256.h>

#include <string>

#include "config.h"
#include "core/pin_policy.h"

namespace lapguard {
namespace {
Preferences preferences;
String stored_salt_hex;
String stored_pin_hash_hex;
uint8_t fail_count = 0;
unsigned long lockout_until_ms = 0;

constexpr size_t kSaltBytes = 16;

int hex_value(char character) {
  if (character >= '0' && character <= '9') {
    return character - '0';
  }
  if (character >= 'a' && character <= 'f') {
    return 10 + (character - 'a');
  }
  if (character >= 'A' && character <= 'F') {
    return 10 + (character - 'A');
  }
  return -1;
}

String bytes_to_hex(const uint8_t* data, size_t length) {
  static const char kHexDigits[] = "0123456789abcdef";
  String result;
  result.reserve(length * 2);
  for (size_t index = 0; index < length; ++index) {
    const uint8_t value = data[index];
    result += kHexDigits[(value >> 4) & 0x0F];
    result += kHexDigits[value & 0x0F];
  }
  return result;
}

bool hex_to_bytes(const String& hex, uint8_t* output, size_t output_length) {
  if (hex.length() != output_length * 2) {
    return false;
  }

  for (size_t index = 0; index < output_length; ++index) {
    const int high = hex_value(hex[index * 2]);
    const int low = hex_value(hex[index * 2 + 1]);
    if (high < 0 || low < 0) {
      return false;
    }
    output[index] = static_cast<uint8_t>((high << 4) | low);
  }

  return true;
}

String generate_salt_hex() {
  uint8_t salt[kSaltBytes];
  for (size_t index = 0; index < kSaltBytes; ++index) {
    salt[index] = static_cast<uint8_t>(esp_random() & 0xFFU);
  }
  return bytes_to_hex(salt, sizeof(salt));
}

String hash_pin_with_salt(const String& salt_hex, const String& pin) {
  uint8_t salt[kSaltBytes];
  if (!hex_to_bytes(salt_hex, salt, sizeof(salt))) {
    return String();
  }

  uint8_t digest[32];
  mbedtls_sha256_context context;
  mbedtls_sha256_init(&context);
  if (mbedtls_sha256_starts_ret(&context, 0) != 0) {
    mbedtls_sha256_free(&context);
    return String();
  }
  mbedtls_sha256_update_ret(&context, salt, sizeof(salt));
  mbedtls_sha256_update_ret(&context, reinterpret_cast<const unsigned char*>(pin.c_str()), pin.length());
  mbedtls_sha256_finish_ret(&context, digest);
  mbedtls_sha256_free(&context);

  return bytes_to_hex(digest, sizeof(digest));
}

bool material_is_valid() {
  return stored_salt_hex.length() == kSaltBytes * 2 && stored_pin_hash_hex.length() == 64;
}

void persist_material() {
  preferences.putString("salt", stored_salt_hex);
  preferences.putString("hash", stored_pin_hash_hex);
}

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
  stored_salt_hex = preferences.getString("salt", "");
  stored_pin_hash_hex = preferences.getString("hash", "");

  if (!material_is_valid()) {
    stored_salt_hex = generate_salt_hex();
    stored_pin_hash_hex = hash_pin_with_salt(stored_salt_hex, DEFAULT_PIN);
    persist_material();
  }

  fail_count = 0;
  lockout_until_ms = 0;
  Serial.println(F("[AUTH] Initialized (SHA256 + salt)"));
}

bool auth_is_valid_pin(const String& pin) {
  const std::string pin_text = pin.c_str();
  return is_valid_pin_format(pin_text, PIN_MIN_LEN, PIN_MAX_LEN);
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

  const String candidate_hash = hash_pin_with_salt(stored_salt_hex, pin);
  if (candidate_hash.length() > 0 && constant_time_equal(candidate_hash, stored_pin_hash_hex)) {
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

  stored_salt_hex = generate_salt_hex();
  stored_pin_hash_hex = hash_pin_with_salt(stored_salt_hex, new_pin);
  persist_material();
  auth_reset_failures();
  Serial.println(F("[AUTH] PIN updated"));
  return true;
}
}  // namespace lapguard