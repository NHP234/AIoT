#pragma once

#include <Arduino.h>

namespace lapguard {
void wifi_init();
void wifi_poll();
bool wifi_is_connected();
int wifi_rssi();
}  // namespace lapguard