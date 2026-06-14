#ifndef FIREBASE_MGR_H
#define FIREBASE_MGR_H

#include <Arduino.h>

namespace lapguard {
void firebase_init();
void firebase_poll();
void firebase_send_alert(float delta_g);
void firebase_update_status(const String& status_str);
void firebase_update_battery_and_rssi(uint8_t battery, int rssi);
}  // namespace lapguard

#endif  // FIREBASE_MGR_H
