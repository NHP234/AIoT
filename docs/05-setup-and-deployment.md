# 05 - Cài đặt và triển khai

## Mục lục

- [1. Yêu cầu trước khi bắt đầu](#1-yêu-cầu-trước-khi-bắt-đầu)
- [2. Phần A - Tạo Telegram Bot](#2-phần-a---tạo-telegram-bot)
- [3. Phần B - Cài môi trường phát triển](#3-phần-b---cài-môi-trường-phát-triển)
- [4. Phần C - Lắp mạch phần cứng](#4-phần-c---lắp-mạch-phần-cứng)
- [5. Phần D - Cấu hình và nạp firmware](#5-phần-d---cấu-hình-và-nạp-firmware)
- [6. Phần E - Kiểm thử lần đầu](#6-phần-e---kiểm-thử-lần-đầu)
- [7. Phần F - Triển khai thực tế](#7-phần-f---triển-khai-thực-tế)
- [8. Troubleshooting](#8-troubleshooting)
- [9. Checklist bàn giao](#9-checklist-bàn-giao)

---

## 1. Yêu cầu trước khi bắt đầu

### Phần cứng

Chuẩn bị đầy đủ linh kiện theo [02-hardware.md](02-hardware.md#2-bảng-vật-tư-bill-of-materials) + dụng cụ:

- Mỏ hàn 30-40W + thiếc + flux.
- Kìm cắt, kìm tuốt dây.
- Đồng hồ vạn năng (để đo điện áp pin, thông mạch).
- Cáp USB Micro-B (cho ESP32) và cáp USB-C (cho TP4056).

### Phần mềm

- Máy tính chạy Windows 10/11, macOS, hoặc Linux.
- VSCode + PlatformIO extension.
- Git (để clone repo).
- Trình duyệt web để dùng Telegram web.
- Smartphone đã cài **Telegram** và đăng nhập.
- WiFi 2.4 GHz có internet (ESP32 không kết nối được 5 GHz).

### Kiến thức nền

- Đọc hiểu sơ đồ mạch cơ bản.
- Biết dùng breadboard và jumper wire.
- Biết mở terminal / cmd để chạy lệnh.

## 2. Phần A - Tạo Telegram Bot

### A.1 Tạo bot với @BotFather

1. Mở Telegram, tìm kiếm `@BotFather`. Nhớ nhìn icon xanh có check mới là bot chính chủ.
2. Bấm **Start**, gõ `/newbot`.
3. Đặt tên hiển thị, ví dụ `LapGuard Demo`.
4. Đặt username kết thúc bằng `bot`, ví dụ `lapguard_iot_bot`.
5. BotFather sẽ trả về **token** dạng `1234567890:AAE-xxxxxxxxxxxxxxxxxxxxx`. **Lưu lại cẩn thận**, đây là chìa khoá điều khiển bot.

### A.2 Tắt privacy mode (khuyến nghị)

Nếu bạn dùng bot trong group, gõ `/setprivacy` ở BotFather, chọn bot, chọn **Disable** để bot nhận được mọi message trong group. Với use case cá nhân thì không cần.

### A.3 Lấy chat_id của bạn

Có 2 cách:

**Cách 1: dùng bot `@userinfobot`**

1. Tìm `@userinfobot` trong Telegram.
2. Bấm Start.
3. Bot trả về JSON có trường `Id`. Đây chính là `chat_id` của bạn.

**Cách 2: dùng API trực tiếp**

1. Gửi tin nhắn bất kỳ (ví dụ "hello") cho bot mới tạo của bạn.
2. Mở browser, truy cập:
   ```
   https://api.telegram.org/bot<TOKEN>/getUpdates
   ```
   thay `<TOKEN>` bằng token ở A.1.
3. Tìm trường `"from":{"id":...}`. Đây là `chat_id`.

### A.4 Test bot

Gõ `/start` trong chat với bot. Nếu bot chưa được lập trình, sẽ không có phản hồi - điều này là bình thường, bot chỉ hoạt động khi firmware chạy.

## 3. Phần B - Cài môi trường phát triển

### B.1 Cài VSCode

Tải từ https://code.visualstudio.com/ và cài bình thường.

### B.2 Cài PlatformIO extension

1. Mở VSCode.
2. Vào tab Extensions (Ctrl+Shift+X).
3. Tìm "PlatformIO IDE", bấm **Install**.
4. Đợi khoảng 2-5 phút để PlatformIO tải core. Sau khi xong VSCode cần khởi động lại.

### B.3 Cài driver USB cho ESP32

Tuỳ board dùng chip nào:

- **CP2102** (phổ biến): tải Silicon Labs CP210x driver từ trang chính chủ.
- **CH340/CH9102**: tải WCH CH340 driver (hoặc CH9102 driver) từ wch.cn.
- **macOS và Linux**: thường tự nhận, không cần driver.

Sau khi cài, cắm ESP32 vào máy, vào Device Manager (Windows) xem có **COMx** mới xuất hiện là OK.

### B.4 Clone repo dự án

```bash
git clone <url-repo-cua-ban>
cd AIoT/firmware
```

Nếu chưa có repo, ở giai đoạn này bạn đang lập kế hoạch nên chỉ cần tạo thư mục `firmware/` trống. Code sẽ được viết ở pha sau theo đặc tả tại [04-firmware-design.md](04-firmware-design.md).

## 4. Phần C - Lắp mạch phần cứng

### C.1 Sơ đồ tổng

Xem chi tiết tại [02-hardware.md mục 6](02-hardware.md#6-sơ-đồ-nối-dây-chi-tiết).

### C.2 Quy trình lắp ráp khuyến nghị

Lắp từng khối, test từng khối. Đừng lắp hết rồi test lần cuối, sẽ khó tìm lỗi.

1. **Bước 1 - Nguồn**:
   - Gắn pin 18650 vào holder.
   - Nối pin -> TP4056 -> MT3608 -> công tắc.
   - Chỉnh biến trở MT3608, đo đầu ra đúng **5.0V +- 0.1V** thì dừng.
   - Chưa nối ESP32 vội.

2. **Bước 2 - ESP32**:
   - Cắm ESP32 vào breadboard.
   - Nối VIN từ MT3608 vào VIN của ESP32, GND chung.
   - Bật công tắc, đèn nguồn đỏ trên ESP32 phải sáng.
   - Cắm USB Micro-B từ laptop vào ESP32, mở Serial Monitor 115200, thấy thông điệp boot là OK.

3. **Bước 3 - Cảm biến MPU6050**:
   - Nối 4 chân: VCC 3V3, GND, SDA 21, SCL 22.
   - Chạy sketch test `Wire.h` scan I2C, phải thấy thiết bị ở địa chỉ `0x68`.
   - Nối thêm INT -> GPIO 15 nếu muốn dùng hardware interrupt.

4. **Bước 4 - SW-420 (Trì hoãn - Tuỳ chọn)**:
   - Bước này đã được trì hoãn sang phiên bản sau. Để trống chân GPIO 14 làm chân dự phòng.

5. **Bước 5 - Đầu ra**:
   - Buzzer: VCC 5V (từ VIN), GND, IN -> GPIO 25.
   - LED xanh: GPIO 26 -> điện trở 220 Ohm -> anode -> cathode -> GND.
   - LED đỏ: tương tự GPIO 27.
   - Test blink từng chân để xác nhận.

6. **Bước 6 - Đo pin** (tuỳ chọn):
   - Cầu phân áp 2 điện trở 100k.
   - Đầu ra cầu phân áp -> GPIO 34.
   - Đọc `analogRead(34)`, nhân hệ số, kiểm tra đúng với đồng hồ vạn năng.

### C.3 Ảnh lắp mạch hoàn chỉnh

<!-- TODO: Chup anh lap xong len breadboard -->

![breadboard done placeholder](../assets/images/breadboard-done.jpg)

## 5. Phần D - Cấu hình và nạp firmware

### D.1 Tạo file `secrets.h`

Trong `firmware/src/`:

```bash
cp secrets.example.h secrets.h
```

Mở `secrets.h`, điền:

```cpp
#define WIFI_SSID       "TenWiFiCuaBan"
#define WIFI_PASSWORD   "MatKhauWiFi"
#define BOT_TOKEN       "1234567890:AAE-xxxxxxxxxxxxxxx"
#define CHAT_ID_OWNER   "987654321"
#define DEFAULT_PIN     "1234"
```

> Đảm bảo `secrets.h` đã được thêm vào `.gitignore` để không lộ lên GitHub.

### D.2 Build firmware

Trong VSCode:

1. Mở thư mục `firmware/` dưới dạng PlatformIO project.
2. Bấm biểu tượng **Build** (dấu check) ở thanh status. Lần đầu sẽ tải thư viện, chờ 3-10 phút.
3. Build xong sẽ thấy `SUCCESS` và kích thước firmware (flash usage).

Hoặc chạy terminal:

```bash
pio run -e esp32dev
```

### D.3 Nạp firmware

1. Cắm USB Micro-B vào ESP32.
2. Xác định cổng COM: PlatformIO -> Devices, hoặc lệnh `pio device list`.
3. Bấm biểu tượng **Upload** (mũi tên phải).
4. Chờ đến khi thấy `Writing... (100%) Hash of data verified`.

Hoặc terminal:

```bash
pio run -t upload -e esp32dev
```

### D.4 Mở Serial Monitor

```bash
pio device monitor -b 115200
```

Hoặc bấm biểu tượng ổ cắm trong VSCode status bar.

Log dự kiến:

```
[BOOT] LapGuard v0.1.0 starting...
[BOOT] Chip: ESP32, cores: 2, flash: 4MB
[WIFI] Connecting to TenWiFi...
[WIFI] Connected, IP: 192.168.1.42, RSSI: -52 dBm
[I2C]  Scanning... found 0x68 (MPU6050)
[MPU]  Init OK
[AUTH] PIN loaded from NVS
[TG]   Bot initialized, chat_id whitelist: 987654321
[FSM]  State -> DISARMED
[BOOT] Ready.
```

## 6. Phần E - Kiểm thử lần đầu

### E.1 Test kết nối Telegram

1. Mở Telegram trên điện thoại.
2. Vào chat với bot của bạn.
3. Gõ `/start`. Bot phải trả về menu.
4. Gõ `/status`. Bot phải trả về state hiện tại, RSSI, uptime.

Nếu bot không phản hồi -> xem mục Troubleshooting 8.2.

### E.2 Test arm / disarm

1. Gõ `/arm 1234`. Bot trả về "ARMED". LED xanh chuyển sang chớp chậm.
2. Gõ `/disarm 1234`. Bot trả về "DISARMED". LED xanh sáng liên tục.
3. Gõ `/arm 9999` (sai PIN). Bot trả về "PIN sai, con 2 lan".
4. Gõ sai 3 lần, bot khoá 30 giây.

### E.3 Test phát hiện chuyển động

1. Gõ `/arm 1234`.
2. Nhấc nhẹ breadboard lên 5 cm.
3. Trong vòng 1 giây: còi kêu + LED đỏ sáng + Telegram nhận được tin nhắn.
4. Gõ `/disarm 1234`. Còi tắt.
5. Lặp lại với: xoay nghiêng, rung mạnh, gõ mạnh mặt bàn.

### E.4 Tinh chỉnh ngưỡng

Nếu còi kêu với rung nhẹ (gõ bàn):

- Tăng `MOTION_THRESHOLD_G` trong `config.h` (ví dụ 0.3 -> 0.5).
- Hoặc tăng `MOTION_PERSISTENCE_N` (3 -> 5).

Nếu còi không kêu khi nhấc laptop:

- Giảm `MOTION_THRESHOLD_G`.

### E.5 Test mất WiFi

1. Arm thiết bị, WiFi đang hoạt động.
2. Rút nguồn WiFi router, hoặc cho ESP32 đến khu vực không có WiFi.
3. LED chuyển chế độ chớp cam.
4. Rung cảm biến -> còi vẫn kêu (local alarm).
5. Cắm lại WiFi. Thiết bị tự reconnect trong 10-30s.
6. Telegram nhận được tin nhắn tóm tắt các event đã xảy ra trong thời gian offline.

## 7. Phần F - Triển khai thực tế

### F.1 Đóng hộp

1. Cắt gọn dây, hàn lên perfboard hoặc giữ trên breadboard mini.
2. Lắp vào vỏ hộp (xem [02-hardware.md mục 9](02-hardware.md#9-vỏ-hộp)).
3. Khoan lỗ USB-C, lỗ công tắc, lỗ còi, lỗ LED.
4. Dán velcro mặt sau.

### F.2 Sạc pin

1. Tắt công tắc chính.
2. Cắm cáp USB-C vào module TP4056.
3. Đèn đỏ trên TP4056 sáng -> đang sạc. Đèn xanh -> đầy.
4. Thời gian sạc đầy pin 2500 mAh ở 1A: ~3 giờ.

### F.3 Sử dụng thực tế

1. Bật công tắc, chờ 10-15s để ESP32 boot và kết nối WiFi.
2. Đặt thiết bị dưới laptop, dán bằng velcro.
3. Mở Telegram, gõ `/status` để xác nhận thiết bị online.
4. Khi rời chỗ: `/arm <PIN>`.
5. Khi quay lại: `/disarm <PIN>` rồi mới chạm laptop.

## 8. Troubleshooting

### 8.1 ESP32 không nhận trên máy tính

- Cắm cáp USB Micro-B khác (đảm bảo cáp có dây data, không phải cáp chỉ sạc).
- Cài đúng driver CP210x hoặc CH340.
- Kiểm tra Device Manager có COM port không.

### 8.2 Bot không phản hồi

- Kiểm tra Serial Monitor thấy `[TG] poll ok` không.
- Gõ lại lệnh `/status`. Nếu vẫn không có -> sai `BOT_TOKEN` hoặc `CHAT_ID_OWNER`.
- Dùng browser truy cập `https://api.telegram.org/bot<TOKEN>/getMe` phải trả về JSON `{"ok":true, ...}`. Nếu `{"ok":false}` -> token sai.
- Kiểm tra WiFi có chặn Telegram không (một số WiFi trường có firewall). Thử dùng hotspot 4G để loại trừ.

### 8.3 ESP32 reset liên tục khi bật WiFi

- Nguồn yếu. Đo điện áp VIN khi WiFi phát, nếu tụt dưới 4.5V -> nguồn không đủ dòng.
- Thay module tăng áp khác, hoặc cắm qua USB laptop để test loại trừ.
- Thêm tụ 470uF ở VIN để lọc.

### 8.4 MPU6050 không phát hiện ở địa chỉ 0x68

- Kiểm tra dây SDA, SCL có bị tráo không.
- Thử chân AD0 nối GND (địa chỉ 0x68) hoặc VCC (địa chỉ 0x69).
- Đo thông mạch, có thể jumper lỗi.
- Nếu I2C scanner thấy `0x68` nhưng thư viện MPU6050 vẫn báo lỗi, đọc thanh ghi
  `WHO_AM_I` (`0x75`). Giá trị `0x70` là MPU6500 và cần driver tương thích.

### 8.5 Còi không kêu

- Đo điện áp chân IN của buzzer khi gọi `digitalWrite(PIN_BUZZER, HIGH)`, phải thấy 3.3V.
- Kiểm tra polarity buzzer (chân dài là +).
- Nếu dùng buzzer passive thay vì active, cần PWM. Xem lại loại buzzer.

### 8.6 Telegram tin nhắn đến chậm (> 10s)

- Nguyên nhân thường gặp: ESP32 poll `getUpdates` chu kỳ quá lớn. Giảm xuống 1s.
- Hoặc dùng `getUpdates` với `long polling` (timeout=10s trong API).

### 8.7 False positive khi laptop đứng yên

- Tăng threshold.
- Kéo dài persistence.
- Kiểm tra vỏ có rung theo quạt laptop không.

## 9. Checklist bàn giao

Dùng checklist này khi nộp bài / demo cho giảng viên:

- [ ] Thiết bị bật được và LED xanh sáng trong <15s.
- [ ] `/status` trả về chính xác.
- [ ] `/arm <PIN>` + nhấc thiết bị -> còi kêu + Telegram nhận thông báo trong <3s.
- [ ] `/disarm <PIN>` tắt còi.
- [ ] `/silence <PIN>` tắt còi nhưng vẫn ARMED.
- [ ] Nhập sai PIN 3 lần -> khoá 30s.
- [ ] Ngắt WiFi -> LED đổi màu -> còi vẫn kêu khi trigger.
- [ ] Chạy pin liên tục >= 8 giờ.
- [ ] Vỏ hộp đóng gọn, có nhãn dán tên nhóm.
- [ ] Slide thuyết trình + video demo sẵn sàng.
- [ ] Mã nguồn đã push lên GitHub, không chứa `secrets.h`.
