# 04 - Thiết kế firmware

## Mục lục

- [1. Framework và công cụ](#1-framework-và-công-cụ)
- [2. Danh sách thư viện](#2-danh-sách-thư-viện)
- [3. Cấu trúc thư mục code](#3-cấu-trúc-thư-mục-code)
- [4. Mô tả từng module](#4-mô-tả-từng-module)
- [5. Mô hình đa luồng (FreeRTOS tasks)](#5-mô-hình-đa-luồng-freertos-tasks)
- [6. Đặc tả lệnh Telegram Bot](#6-đặc-tả-lệnh-telegram-bot)
- [7. Định dạng tin nhắn cảnh báo](#7-định-dạng-tin-nhắn-cảnh-báo)
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
| `WiFiClientSecure` | built-in | HTTPS cho Telegram API |
| `Preferences` | built-in | NVS lưu PIN hash, config |
| `Wire` | built-in | I2C cho MPU6050 |
| `mbedtls/sha256` | built-in | Hash PIN |
| `UniversalTelegramBot` | 1.3.0+ (Brian Lough) | Wrapper Telegram Bot API |
| `ArduinoJson` | 6.21.0+ | Parse/format JSON |
| Driver I2C nội bộ | trong `motion.cpp` | Hỗ trợ MPU6050 và module thay thế MPU6500 |
| `esp_task_wdt` | built-in | Watchdog |

> Tất cả thư viện bên ngoài được khai báo trong `platformio.ini` ở mục `lib_deps`
> để tự động tải về khi build.

## 3. Cấu trúc thư mục code

```
firmware/
├── platformio.ini                     # cau hinh PlatformIO
├── README.md                          # huong dan chay firmware
├── src/
│   ├── main.cpp                       # entry point, setup() + loop()
│   ├── config.h                       # hang so cau hinh (threshold, pin GPIO)
│   ├── secrets.example.h              # mau cho secrets.h
│   ├── secrets.h                      # (KHONG commit) WiFi, token, chat_id
│   ├── fsm/
│   │   ├── fsm.h
│   │   └── fsm.cpp                    # finite state machine
│   ├── sensor/
│   │   ├── motion.h
│   │   └── motion.cpp                 # MPU6050 + SW-420
│   ├── alarm/
│   │   ├── alarm.h
│   │   └── alarm.cpp                  # buzzer + LED
│   ├── net/
│   │   ├── wifi_mgr.h
│   │   ├── wifi_mgr.cpp               # WiFi connect + reconnect
│   │   ├── telegram_bot.h
│   │   └── telegram_bot.cpp           # gui/nhan lenh
│   ├── auth/
│   │   ├── auth.h
│   │   └── auth.cpp                   # PIN hash, rate-limit
│   └── power/
│       ├── battery.h
│       └── battery.cpp                # doc ADC, canh bao pin yeu
├── test/                              # unit test (native)
│   └── test_auth/
│       └── test_auth.cpp
└── lib/                               # thu vien custom (neu co)
```

## 4. Mô tả từng module

### `main.cpp`

Chịu trách nhiệm:

- Khởi tạo Serial, GPIO, I2C, cảm biến, WiFi, Telegram, NVS.
- Tạo các FreeRTOS task: `SensorTask`, `TelegramTask`, `FSMTask`, `BatteryTask`.
- Trong `loop()` chỉ feed watchdog và delay nhỏ.

### `config.h`

Hằng số cấu hình public cho toàn project:

```cpp
// GPIO
#define PIN_BUZZER      25
#define PIN_LED_GREEN   26
#define PIN_LED_RED     27
// #define PIN_SW420       14 (Option v2 - Trì hoãn)
#define PIN_MPU_INT     15
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

// Auth
#define PIN_MIN_LEN            4
#define PIN_MAX_LEN            8
#define AUTH_MAX_FAILS         3
#define AUTH_LOCKOUT_MS        30000

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

### `sensor/motion.{h,cpp}`

- Tự nhận diện MPU6050 (`WHO_AM_I=0x68`) hoặc MPU6500 (`0x70`), cấu hình ở
  `+-4g`, gyro `+-500 deg/s`, filter DLPF khoảng 44Hz.
- Cấu hình MPU6050 interrupt motion detection hardware (tuỳ chọn nâng cao).
- Hàm `motion_sample()`: đọc gia tốc, tính delta, đẩy vào ring buffer, trả về `true` nếu vượt ngưỡng trong N mẫu.
- Hàm `motion_check()` được `SensorTask` gọi định kỳ, phát `EVT_MOTION` tới FSM.
- *Lưu ý: Hỗ trợ cảm biến rung SW-420 đã bị trì hoãn.*

### `alarm/alarm.{h,cpp}`

- `alarm_init()`: set GPIO output, tắt hết.
- `alarm_on()`: bật buzzer, bật LED đỏ, start timer 60s.
- `alarm_off()`: tắt buzzer, LED đỏ.
- `led_set_state(State s)`: điều khiển LED xanh/đỏ theo state (on liên tục, chớp chậm, chớp SOS).
- Dùng `ledcWrite` nếu muốn còi hú theo pattern (thay đổi tần số PWM).

### `net/wifi_mgr.{h,cpp}`

- `wifi_begin()`: thử connect trong 10s, nếu fail -> OFFLINE.
- Task reconnect chạy nền: nếu `WiFi.status() != WL_CONNECTED` thì retry mỗi 10s.
- Phát event `EVT_WIFI_UP` / `EVT_WIFI_DOWN` tới FSM.

### `net/telegram_bot.{h,cpp}`

- `tg_init(token)`: khởi tạo `UniversalTelegramBot` với `WiFiClientSecure`.
- `tg_poll()`: gọi mỗi giây, lấy updates, gọi `on_command(chat_id, text)`.
- `tg_send_alert(float delta)`: format message + gửi.
- `tg_send_status()`: gửi state hiện tại + RSSI + battery.
- Queue offline: `std::vector<OfflineEvent> queue` (giới hạn 20), flush khi WiFi up.

### `auth/auth.{h,cpp}`

- `auth_init()`: đọc PIN hash + salt từ NVS, nếu chưa có thì set default PIN = "1234", hash và lưu.
- `auth_verify_pin(const String& pin)`: hash và so sánh constant-time với hash lưu.
- `auth_change_pin(const String& old_pin, const String& new_pin)`: verify old, validate new (4-8 digit), update NVS.
- `auth_is_locked()`: kiểm tra có đang trong thời gian khoá không.
- `auth_record_fail()`: tăng counter, nếu >= MAX_FAILS thì set `lockout_until = millis() + 30000`.

### `power/battery.{h,cpp}`

- `battery_read_mv()`: đọc ADC1 trên GPIO 34, nhân hệ số phân áp, trả về mV.
- Chạy mỗi 60s trong `BatteryTask`.
- Nếu dưới `BAT_LOW_MV` và chưa gửi cảnh báo -> gửi Telegram + set flag.
- Nếu dưới `BAT_CRITICAL_MV` -> lưu state NVS rồi `esp_deep_sleep_start()`.

## 5. Mô hình đa luồng (FreeRTOS tasks)

ESP32 dual-core, chia task để tránh I/O chặn:

| Task | Core | Priority | Stack | Chu kỳ | Trách nhiệm |
|------|------|----------|-------|--------|-------------|
| `SensorTask` | 1 | 5 | 4 KB | 20 ms | Đọc MPU6050, kiểm tra motion |
| `TelegramTask` | 0 | 3 | 10 KB | 1 s | Poll Telegram, gửi tin nhắn |
| `FSMTask` | 1 | 4 | 4 KB | event-driven (queue) | Xử lý sự kiện, chuyển state |
| `BatteryTask` | 0 | 1 | 2 KB | 60 s | Đọc pin |

Giao tiếp giữa task qua **FreeRTOS queue**:

- `xEventQueue`: chứa `Event` enum, SensorTask & TelegramTask đẩy vào, FSMTask consume.

## 6. Đặc tả lệnh Telegram Bot

| Lệnh | Tham số | Điều kiện | Phản hồi thành công | Phản hồi thất bại |
|------|---------|-----------|---------------------|-------------------|
| `/start` | không | luôn | giới thiệu + help | - |
| `/help` | không | luôn | danh sách lệnh | - |
| `/arm <PIN>` | PIN | state = DISARMED & PIN đúng | "ARMED, dang giam sat" | "PIN sai" / "Dang khoa 30s" |
| `/disarm <PIN>` | PIN | state = ARMED/TRIGGERED & PIN đúng | "DISARMED" | "PIN sai" |
| `/silence <PIN>` | PIN | state = TRIGGERED & PIN đúng | "Coi tat, van giam sat" | "PIN sai" / "Khong o TRIGGERED" |
| `/status` | không | chat_id whitelist | `state`, RSSI, pin mV, uptime | - |
| `/setpin <old> <new>` | PIN cũ + mới | PIN cũ đúng & PIN mới valid | "Doi PIN thanh cong" | "PIN cu sai" / "PIN moi khong hop le" |
| `/threshold <value>` | 0.1 - 2.0 | admin only | "Threshold moi: X g" | giá trị ngoài khoảng |
| `/reboot <PIN>` | PIN | PIN đúng | "Reboot..." | "PIN sai" |

### Ví dụ tương tác

```text
User:   /start
Bot:    LapGuard chao ban! Cac lenh:
        /arm <PIN>     - bat giam sat
        /disarm <PIN>  - tat giam sat
        /silence <PIN> - tat coi, giu giam sat
        /status        - xem trang thai
        /setpin        - doi PIN

User:   /arm 1234
Bot:    OK, da chuyen sang ARMED luc 09:45:12. Chuc ban an tam.

User:   /status
Bot:    State: ARMED
        WiFi: -52 dBm (tot)
        Pin: 3950 mV (80%)
        Uptime: 2h15m

User:   /disarm 0000
Bot:    PIN sai. Con 2 lan truoc khi bi khoa.
```

## 7. Định dạng tin nhắn cảnh báo

```text
CANH BAO CHONG TROM!
--------------------
Thoi gian: 2026-05-04 14:23:17
Muc do rung: 3.2 g (MANH)
Trigger: MPU6050 (accel)
RSSI: -58 dBm
Pin: 3920 mV

Gui /disarm <PIN> de tat coi.
```

Khi pin yếu:

```text
CANH BAO PIN YEU
----------------
Dien ap: 3350 mV (~15%)
Xin sac lai de tranh mat giam sat.
```

Khi WiFi có lại sau offline:

```text
[WiFi tro lai]
Trong 3m22s offline da xay ra:
- 14:21:05 MOTION delta=2.1g (coi da keu 42s)
- 14:22:10 MOTION delta=1.5g (tai phat)

State hien tai: ARMED
```

## 8. Cấu hình qua file secrets

### `src/secrets.example.h` (commit vào git, làm mẫu)

```cpp
#pragma once

#define WIFI_SSID       "YourWiFiName"
#define WIFI_PASSWORD   "YourWiFiPassword"

#define BOT_TOKEN       "1234567890:AAE..."
#define CHAT_ID_OWNER   "123456789"

#define DEFAULT_PIN     "1234"

#define DEVICE_NAME     "LapGuard-01"
```

### `src/secrets.h` (KHÔNG commit - thêm vào `.gitignore`)

Sao chép từ `secrets.example.h` và điền giá trị thật.

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
    witnessmenow/UniversalTelegramBot @ ^1.3.0
    bblanchon/ArduinoJson @ ^6.21.3

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
- Hàm public có doxygen-style comment ngắn:

```cpp
/// Kiem tra PIN nguoi dung nhap.
/// @param pin chuoi 4-8 chu so.
/// @return true neu khop, false neu sai hoac bi khoa.
bool auth_verify_pin(const String& pin);
```

- Không dùng `String` cho dữ liệu lớn (gây phân mảnh heap), chuyển sang `std::string` hoặc buffer cố định.
- Log dùng `Serial.printf()` có tag: `[SENSOR]`, `[FSM]`, `[TG]`, `[AUTH]`.
- Mỗi module phải có ít nhất 1 unit test native (nếu logic không phụ thuộc Arduino).

<!-- TODO: Khi implement code thuc te, cap nhat tai lieu nay neu co thay doi -->
