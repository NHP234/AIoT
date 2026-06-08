#pragma once

#include <Arduino.h>

namespace lapguard {
void telegram_init();
bool telegram_send_text(const String& chat_id, const String& text);
bool telegram_send_alert(float delta_g);
bool telegram_send_status();
}  // namespace lapguard
