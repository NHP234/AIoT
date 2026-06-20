# 04 - Thiết kế firmware

## Mục lục

- [1. Framework và công cụ](#1-framework-và-công-cụ)
- [2. Danh sách thư viện](#2-danh-sách-thư-viện)
- [3. Cấu trúc thư mục code](#3-cấu-trúc-thư-mục-code)
- [4. Mô tả từng module](#4-mô-tả-từng-module)
- [5. Mô hình vòng lặp và hàng đợi Firebase](#5-mô-hình-vòng-lặp-và-hàng-đợi-firebase)
- [6. Đặc tả cấu trúc dữ liệu và đồng bộ lệnh](#6-đặc-tả-cấu-trúc-dữ-liệu-và-đồng-bộ-lệnh-firebase-database)
- [7. Định dạng gói tin nhật ký báo động](#7-định-dạng-gói-tin-nhật-ký-báo-động-json-logs)
- [8. Cấu hình qua file secrets](#8-cấu-hình-qua-file-secrets)
- [9. Cấu hình PlatformIO](#9-cấu-hình-platformio)
- [10. Quy ước code](#10-quy-ước-code)

---

## 1. Framework và công cụ

- **Framework**: Arduino-ESP32 (arduino-espressif32 core, phiên bản >= 2.0.14).
- **Build system**: **PlatformIO** (khuyến nghị) hoặc Arduino IDE 2.x.
- **Ngôn ngữ**: C++ 11, style tương tự Arduino.
- **IDE**: VSCode + PlatformIO extension.
- **Debug**: Serial Monitor 115200 baud, thư viện `esp_log` khi cần log có level.

Lý do chọn PlatformIO thay vì Arduino IDE:

- Quản lý thư viện qua `platformio.ini` đảm bảo tái lập được build.
- Hỗ trợ `lib_deps` pin phiên bản cụ thể.
- Auto-complete, include path tốt hơn trong VSCode.
- Dễ chạy `pio run` trong CI sau này.

## 2. Danh sách thư viện

| Thư viện | Phiên bản | Mục đích |
|----------|-----------|----------|
| `WiFi` | built-in | Kết nối WiFi |
| `Firebase-ESP-Client` | 4.4.0+ (Mobizt) | Giao tiếp thời gian thực với Firebase Database |
| `WiFiManager` | 2.0.16+ (tzapu) | Captive Portal cấu hình WiFi cục bộ |
| `Preferences` | built-in | NVS lưu trữ cấu hình mạng |
| `Wire` | built-in | I2C cho MPU6050/6500 |
| `ArduinoJson` | 6.21.0+ | Parse/format JSON |
| Driver I2C nội bộ | trong `motion.cpp` | Hỗ trợ MPU6050 và MPU6500 |
| `esp_task_wdt` | built-in | Watchdog |

> Tất cả thư viện bên ngoài được khai báo trong `platformio.ini` ở mục `lib_deps`
> để tự động tải về khi build.

## 3. Cấu trúc thư mục code

```
firmware/
├── platformio.ini                     # cau hinh PlatformIO
├── README.md                          # huong dan chay firmware
├── include/
│   └── config.h                       # hang so cau hinh (threshold, pin GPIO)
├── src/
│   ├── main.cpp                       # entry point, setup() + loop()
│   ├── secrets.example.h              # mau cho secrets.h
│   ├── secrets.h                      # (KHONG commit) Firebase keys
│   ├── fsm/
│   │   ├── fsm.h
│   │   └── fsm.cpp                    # finite state machine
│   ├── motion/
│   │   ├── motion.h
│   │   └── motion.cpp                 # MPU6050/6500
│   ├── alarm/
│   │   ├── alarm.h
│   │   └── alarm.cpp                  # buzzer + LED
│   ├── net/
│   │   ├── wifi_mgr.h
│   │   ├── wifi_mgr.cpp               # WiFiManager config
│   │   ├── firebase_mgr.h
│   │   └── firebase_mgr.cpp           # dong bo realtime Firebase
│   └── power/
│       ├── battery.h
│       └── battery.cpp                # doc ADC, canh bao pin yeu
└── lib/                               # thu vien custom (neu co)
```

## 4. Mô tả từng module

### `main.cpp`

Chịu trách nhiệm:

- Khởi tạo Serial, GPIO, I2C, cảm biến, WiFiManager và Firebase.
- Gọi `wifi_poll()`, `alarm_poll()`, `battery_poll()`, `motion_poll()` và `firebase_poll()` trong vòng `loop()` chính.
- Đọc motion trước khi xử lý hàng đợi Firebase để giảm nguy cơ thao tác mạng làm trễ phát hiện chuyển động.

### `config.h`

Hằng số cấu hình public cho toàn project:

```cpp
// GPIO
#define PIN_BUZZER      25
#define PIN_LED_GREEN   26
#define PIN_LED_RED     27
#define PIN_MPU_INT     13
#define PIN_BATTERY_ADC 34

// I2C
#define I2C_SDA         21
#define I2C_SCL         22

// Motion thresholds
#define MOTION_THRESHOLD_G     0.30f
#define MOTION_PERSISTENCE_N   3
#define MOTION_SAMPLE_HZ       50

// Alarm
#define ALARM_MAX_DURATION_MS  60000
#define ALARM_REARM_DEAD_MS    10000

// Battery
#define BAT_LOW_MV             3400
#define BAT_CRITICAL_MV        3000
```

### `fsm/fsm.{h,cpp}`

- Enum `State { BOOT, DISARMED, ARMED, TRIGGERED, OFFLINE }`.
- Enum `Event { EVT_MOTION, EVT_CMD_ARM, EVT_CMD_DISARM, EVT_CMD_SILENCE, EVT_WIFI_UP, EVT_WIFI_DOWN, EVT_ALARM_TIMEOUT }`.
- Hàm `fsm_transition(Event e)` chuyển trạng thái theo bảng ở [03-system-architecture.md](03-system-architecture.md#5-bảng-chuyển-trạng-thái).
- Callback `on_enter_state(State s)` để gọi bật/tắt còi, LED, gửi notification.
- Biến `current_state` bảo vệ bằng mutex hoặc `portENTER_CRITICAL` (vì truy cập từ nhiều task).

### `motion/motion.{h,cpp}`

- Tự nhận diện MPU6050 (`WHO_AM_I=0x68`) hoặc MPU6500 (`0x70`), cấu hình ở
  `+-4g`, gyro `+-500 deg/s`, filter DLPF khoảng 44Hz.
- Cấu hình MPU6050 interrupt motion detection hardware (tuỳ chọn nâng cao).
- Hàm `motion_sample()`: đọc gia tốc, tính delta, đẩy vào ring buffer, trả về `true` nếu vượt ngưỡng trong N mẫu.
- Hàm `motion_poll()` tự giới hạn chu kỳ đọc 20 ms, trả về `true` khi motion vượt ngưỡng trong đủ số mẫu liên tiếp.

### `alarm/alarm.{h,cpp}`

- `alarm_init()`: set GPIO output, tắt hết.
- `alarm_on()`: bật buzzer, bật LED đỏ, start timer 60s.
- `alarm_off()`: tắt buzzer, LED đỏ.
- `led_set_state(State s)`: điều khiển LED xanh/đỏ theo state (on liên tục, chớp chậm, chớp SOS).
- Dùng `ledcWrite` nếu muốn còi hú theo pattern (thay đổi tần số PWM).

### `net/wifi_mgr.{h,cpp}`

- Tích hợp thư viện `WiFiManager` để tự động phát WiFi `LapGuard_AP` khi không có WiFi kết nối được.
- Tùy biến giao diện HTML/CSS (Dark Mode, Bo tròn nút bấm) trong `WiFiManager` để đồng bộ thẩm mỹ với React Web App.
- Phát event `EVT_WIFI_UP` / `EVT_WIFI_DOWN` tới FSM.

### `net/firebase_mgr.{h,cpp}`

- `firebase_init()`: khởi tạo cấu hình kết nối Firebase Realtime Database bằng Token/API Key.
- `firebase_poll()`: duy trì stream lắng nghe nhánh `/devices/<MAC>/command`, xử lý lệnh `"ARM"`, `"DISARM"`, `"SILENCE"` và ghi đè lại `"NONE"` sau khi nhận lệnh.
- `firebase_send_alert(float delta)`: đưa cảnh báo motion vào hàng đợi RAM; `firebase_poll()` sẽ đẩy lên `/logs` khi Firebase sẵn sàng.
- `firebase_update_status()`: chỉ lưu trạng thái mới nhất vào hàng đợi nội bộ, tránh gọi mạng trực tiếp từ FSM.
- `firebase_update_battery_and_rssi()`: đồng bộ pin, RSSI và `last_seen` định kỳ.

### `power/battery.{h,cpp}`

- `battery_read_mv()`: đọc ADC1 trên GPIO 34, nhân hệ số phân áp, trả về mV.
- `battery_poll()` đọc lại mỗi 60s trong vòng `loop()` chính.
- Nếu dưới `BAT_LOW_MV` và chưa gửi cảnh báo -> ghi log pin yếu lên Firebase khi đang có kết nối.
- Nếu dưới `BAT_CRITICAL_MV` -> lưu state, gửi cảnh báo khẩn cấp rồi `esp_deep_sleep_start()`.

## 5. Mô hình vòng lặp và hàng đợi Firebase

Firmware hiện dùng vòng `loop()` Arduino, nhưng đã tách các thao tác Firebase khỏi đường xử lý FSM/motion:

| Thành phần | Chu kỳ | Trách nhiệm |
|------------|--------|-------------|
| `motion_poll()` | 20 ms nội bộ khi armed | Đọc MPU6050/6500, lọc delta, phát hiện motion |
| `alarm_poll()` | mỗi vòng loop | Điều khiển LED/còi và timeout báo động |
| `battery_poll()` | 60 s | Đọc điện áp pin |
| `firebase_poll()` | mỗi vòng loop | Duy trì stream command, xử lý hàng đợi ghi Firebase |

Các thao tác ghi Firebase được gom vào hàng đợi nội bộ:

- Status: chỉ giữ trạng thái mới nhất cần đồng bộ.
- Motion alert: lưu tối đa 8 alert trong RAM để gửi lại khi WiFi/Firebase sẵn sàng.

Hướng nâng cấp sau: tách `SensorTask`, `FirebaseTask`, `FSMTask` bằng FreeRTOS queue nếu cần độ trễ ổn định hơn dưới tải mạng cao.

## 6. Đặc tả cấu trúc dữ liệu và đồng bộ lệnh (Firebase Database)

Các nhánh dữ liệu được ESP32 đồng bộ và phản hồi thời gian thực:

### 6.1 Đồng bộ trạng thái từ ESP32 lên Firebase
* Thiết bị ghi trạng thái của mình lên đường dẫn `/devices/<MAC>/status` (các giá trị: `"DISARMED"`, `"ARMED"`, `"TRIGGERED"`, `"OFFLINE"`).
* Cập nhật định kỳ lượng pin lên `/devices/<MAC>/battery_percent` và cường độ sóng WiFi lên `/devices/<MAC>/wifi_rssi`.

### 6.2 Nhận lệnh điều khiển từ Web App
* Khi người dùng nhấn nút điều khiển trên Web App, app ghi giá trị lệnh tương ứng vào `/devices/<MAC>/command`.
* ESP32 nhận lệnh qua WebSocket, gửi sự kiện tương ứng vào FSM:
  * Nhận `"ARM"` $\rightarrow$ Phát event `EVT_CMD_ARM`.
  * Nhận `"DISARM"` $\rightarrow$ Phát event `EVT_CMD_DISARM`.
  * Nhận `"SILENCE"` $\rightarrow$ Phát event `EVT_CMD_SILENCE`.
* Sau khi nhận lệnh, ESP32 sẽ ghi đè giá trị `"NONE"` lên `/devices/<MAC>/command` để báo hoàn thành.

## 7. Định dạng gói tin nhật ký báo động (JSON Logs)

Mỗi khi phát hiện trộm ở trạng thái ARMED, ESP32 ghi một node mới vào danh sách `/logs/$log_id`:

```json
{
  "device_id": "240AC4123456",
  "timestamp": 1780725100,
  "event_type": "MOTION_ALERT",
  "detail": "Phát hiện chuyển động mạnh (delta = 2.512g)",
  "resolved": false
}
```

Khi người dùng thực hiện tắt còi (DISARM), Web App sẽ cập nhật thuộc tính `"resolved"` của log hiện tại thành `true`.


## 8. Cấu hình qua file secrets

### `src/secrets.example.h` (commit vào git, làm mẫu)

```cpp
#pragma once

#define FIREBASE_API_KEY      "AIzaSyA1..."
#define FIREBASE_DATABASE_URL "https://your-project.firebaseio.com"

#define WIFI_AP_SSID          "LapGuard_AP"
#define WIFI_AP_PASSWORD      "12345678" // Mật khẩu WiFi phát ra để cấu hình

#define DEVICE_NAME           "LapGuard-01"
```

### `src/secrets.h` (KHÔNG commit - thêm vào `.gitignore`)

Sao chép từ `secrets.example.h` và điền giá trị thật của dự án Firebase của bạn.

### `.gitignore` đề xuất

```
firmware/src/secrets.h
firmware/.pio/
firmware/.vscode/
*.bin
*.elf
```

## 9. Cấu hình PlatformIO

Ví dụ `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32 @ ^6.5.0
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 921600

build_flags =
    -DCORE_DEBUG_LEVEL=3
    -DCONFIG_ARDUINO_LOOP_STACK_SIZE=8192

lib_deps =
    mobizt/Firebase ESP32 Client @ ^4.4.14
    bblanchon/ArduinoJson @ ^6.21.3
    https://github.com/tzapu/WiFiManager.git

[env:esp32dev-release]
extends = env:esp32dev
build_flags =
    ${env:esp32dev.build_flags}
    -DNDEBUG
    -Os
```

## 10. Quy ước code

- File: `snake_case.cpp/.h`.
- Hàm: `snake_case()`.
- Class / struct: `PascalCase`.
- Hằng số: `UPPER_SNAKE_CASE`.
- Biến toàn cục: tiền tố `g_`.
- Biến volatile từ ISR: tiền tố `v_`.
- Mỗi file `.h` có `#pragma once`.
- Không dùng `String` cho dữ liệu lớn (gây phân mảnh heap), chuyển sang `std::string` hoặc buffer cố định.
- Log dùng `Serial.printf()` có tag: `[SENSOR]`, `[FSM]`, `[FB]`, `[WIFI]`.
- Mỗi module phải có ít nhất 1 unit test native (nếu logic không phụ thuộc Arduino).

<!-- TODO: Khi implement code thuc te, cap nhat tai lieu nay neu co thay doi -->
