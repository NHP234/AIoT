# LapGuard - Thiết bị IoT chống trộm laptop

> Cảm biến rung + gia tốc phát hiện di chuyển trái phép, kêu còi báo động tại chỗ và gửi thông báo tức thì qua Telegram.

<!-- TODO: chèn ảnh banner/logo dự án vào đây -->

![banner placeholder](assets/images/banner.png)

---

## 1. Thông tin dự án

- **Môn học**: IoT / Ứng dụng IoT
- **Trường**: <!-- TODO: tên trường -->
- **Giảng viên hướng dẫn**: <!-- TODO: tên GVHD -->
- **Học kỳ**: <!-- TODO: học kỳ / năm học -->

### Thành viên nhóm

| STT | Họ và tên | MSSV | Vai trò |
|-----|-----------|------|---------|
| 1   | <!-- TODO --> | <!-- TODO --> | Trưởng nhóm / Firmware |
| 2   | <!-- TODO --> | <!-- TODO --> | Phần cứng / Hàn mạch |
| 3   | <!-- TODO --> | <!-- TODO --> | Tài liệu / Thuyết trình |
| 4   | <!-- TODO --> | <!-- TODO --> | Kiểm thử / Video demo |

---

## 2. Mô tả ngắn

LapGuard là thiết bị nhỏ gọn (khoảng bằng hộp diêm) được đặt dưới hoặc dán cạnh laptop khi
chủ sở hữu rời đi (ví dụ đi vệ sinh, lên nhận đồ ở quầy cà phê, hoặc để quên ở thư viện).
Khi có kẻ lạ nhấc, đẩy hoặc va chạm mạnh vào laptop, thiết bị sẽ:

1. **Kêu còi chói tai** tại chỗ để xua đuổi và thu hút chú ý của người xung quanh.
2. **Gửi thông báo** đến điện thoại của chủ qua **Telegram** kèm thời điểm và mức độ rung.
3. Cho phép chủ **tắt còi / bật lại chế độ giám sát** từ xa chỉ bằng lệnh Telegram có PIN.

---

## 3. Tính năng chính

- Phát hiện chuyển động đa chiều bằng **MPU6050** (gia tốc + con quay hồi chuyển).
- Phát hiện va chạm tức thời bằng **SW-420** (cảm biến rung).
- **Arm / Disarm từ xa** qua Telegram bằng lệnh có PIN.
- **Còi báo động cục bộ** hoạt động kể cả khi mất WiFi.
- **LED trạng thái** hiển thị: mất kết nối / đang giám sát / đang báo động.
- **PIN an toàn**: lưu dạng băm SHA-256 trong NVS, khoá 30 giây sau 3 lần nhập sai.
- **Whitelist chat_id**: chỉ đúng điện thoại chủ mới điều khiển được.
- **Chạy pin 18650** có thể sạc lại qua cổng USB-C (module TP4056).

---

## 4. Công nghệ sử dụng

| Lớp | Công nghệ |
|-----|-----------|
| Vi điều khiển | ESP32 DevKit V1 (Xtensa dual-core, WiFi, BLE) |
| Framework | Arduino-ESP32 trên **PlatformIO** |
| Giao thức cảm biến | I2C (MPU6050), GPIO digital (SW-420) |
| Truyền thông | WiFi 2.4 GHz -> HTTPS -> Telegram Bot API |
| Lưu trữ cấu hình | NVS (`Preferences.h`) |
| Thư viện chính | `UniversalTelegramBot`, `Adafruit_MPU6050`, `ArduinoJson` |
| Công cụ | VSCode, PlatformIO, Fritzing (vẽ mạch), Figma (vỏ hộp) |

---

## 5. Mục lục tài liệu

Toàn bộ tài liệu chi tiết nằm trong thư mục [`docs/`](docs/):

| # | Tài liệu | Nội dung |
|---|----------|----------|
| 1 | [Tổng quan dự án](docs/01-project-overview.md) | Bối cảnh, mục tiêu, phạm vi, yêu cầu FR/NFR |
| 2 | [Phần cứng](docs/02-hardware.md) | BOM, pinout, sơ đồ nối dây |
| 3 | [Kiến trúc hệ thống](docs/03-system-architecture.md) | Block diagram, data flow, state machine |
| 4 | [Thiết kế firmware](docs/04-firmware-design.md) | Cấu trúc code, thư viện, lệnh Telegram |
| 5 | [Cài đặt & triển khai](docs/05-setup-and-deployment.md) | Hướng dẫn từng bước setup HW + SW |
| 6 | [Tiến độ & chi phí](docs/06-timeline-and-budget.md) | Roadmap 6 tuần, bảng chi phí chi tiết |
| 7 | [Kiểm thử & rủi ro](docs/07-testing-and-risks.md) | Test cases, rủi ro, hướng mở rộng |

---

## 6. Demo nhanh

<!-- TODO: quay video demo ~60 giây và dán link YouTube vào đây -->

- Video demo: [YouTube link placeholder](https://youtu.be/xxxxxxxx)
- Ảnh sản phẩm hoàn chỉnh:

![product photo placeholder](assets/images/product.jpg)

- Ảnh chụp màn hình Telegram khi báo động:

![telegram alert placeholder](assets/images/telegram-alert.png)

---

## 7. Bắt đầu nhanh (Quick Start)

```bash
# 1. Clone repo
git clone <repo-url>
cd AIoT

# 2. Cài PlatformIO (VSCode extension) rồi tạo file secrets
cp firmware/src/secrets.example.h firmware/src/secrets.h
# Sửa WIFI_SSID, WIFI_PASS, BOT_TOKEN, CHAT_ID

# 3. Nạp firmware
pio run -t upload -e esp32dev

# 4. Mo Serial Monitor
pio device monitor -b 115200
```

Chi tiết đầy đủ xem tại [docs/05-setup-and-deployment.md](docs/05-setup-and-deployment.md).

---

## 8. Cấu trúc thư mục

```
AIoT/
├── README.md                     # File này
├── docs/                         # 7 file tài liệu chi tiết
├── firmware/                     # Code ESP32 (PlatformIO) - tạo ở pha sau
├── hardware/                     # Schematic, file Fritzing, file in 3D vỏ
└── assets/images/                # Hình minh hoạ cho tài liệu
```

---

## 9. Giấy phép

Dự án học thuật, phát hành theo giấy phép MIT cho mục đích học tập và phi thương mại.

<!-- TODO: thêm file LICENSE nếu cần -->
