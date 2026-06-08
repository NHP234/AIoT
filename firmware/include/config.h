#pragma once

#include <Arduino.h>

#if __has_include("../src/secrets.h")
#include "../src/secrets.h"
#elif __has_include("../src/secrets.example.h")
#include "../src/secrets.example.h"
#endif

namespace lapguard {
constexpr uint8_t PIN_BUZZER = 25;
constexpr uint8_t PIN_LED_GREEN = 26;
constexpr uint8_t PIN_LED_RED = 27;
constexpr uint8_t PIN_MPU_INT = 15;
constexpr uint8_t PIN_BATTERY_ADC = 34;
constexpr uint8_t I2C_SDA = 21;
constexpr uint8_t I2C_SCL = 22;

constexpr float MOTION_THRESHOLD_G = 0.30f;
constexpr uint8_t MOTION_PERSISTENCE_N = 3;
constexpr uint32_t ALARM_MAX_DURATION_MS = 60000UL;
constexpr uint32_t ALARM_REARM_DEAD_MS = 10000UL;
constexpr uint32_t TELEGRAM_POLL_INTERVAL_MS = 500UL;
constexpr uint16_t TELEGRAM_POLL_RESPONSE_MS = 350U;
constexpr uint16_t TELEGRAM_SEND_RESPONSE_MS = 1500U;
constexpr uint8_t TELEGRAM_QUEUE_LENGTH = 8;
constexpr uint16_t TELEGRAM_SEND_TASK_STACK = 12288;
constexpr uint16_t TELEGRAM_POLL_TASK_STACK = 10240;
constexpr uint8_t PIN_MIN_LEN = 4;
constexpr uint8_t PIN_MAX_LEN = 8;
constexpr uint8_t AUTH_MAX_FAILS = 3;
constexpr uint32_t AUTH_LOCKOUT_MS = 30000UL;
constexpr uint16_t BAT_LOW_MV = 3400;
constexpr uint16_t BAT_CRITICAL_MV = 3000;
}  // namespace lapguard
