# 05 - Cài đặt và triển khai

## Mục lục

- [1. Yêu cầu trước khi bắt đầu](#1-yêu-cầu-trước-khi-bắt-đầu)
- [2. Phần A - Thiết lập dự án Firebase & React Web App](#2-phần-a---thiết-lập-dự-án-firebase--react-web-app)
- [3. Phần B - Cài môi trường phát triển](#3-phần-b---cài-môi trường-phát-triển)
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
- Trình duyệt web hiện đại để chạy React Web App.
- Tài khoản Firebase/Google để cấu hình Realtime Database và Authentication.
- WiFi 2.4 GHz có internet (ESP32 không kết nối được 5 GHz).

### Kiến thức nền

- Đọc hiểu sơ đồ mạch cơ bản.
- Biết dùng breadboard và jumper wire.
- Biết mở terminal / cmd để chạy lệnh.

## 2. Phần A - Thiết lập dự án Firebase & React Web App

### A.1 Cấu hình Firebase Console

1. Truy cập [Firebase Console](https://console.firebase.google.com/) và đăng nhập bằng tài khoản Google.
2. Nhấp vào **Create a project** (Tạo dự án mới), đặt tên dự án (ví dụ: `LapGuard-Smart-Alarm`) và nhấp **Continue**.
3. **Bật Authentication**:
   - Ở thanh điều hướng bên trái, chọn **Build** -> **Authentication** -> bấm **Get Started**.
   - Ở mục **Sign-in method**, chọn **Email/Password**, nhấn **Enable** và bấm **Save**.
4. **Bật Realtime Database**:
   - Chọn **Build** -> **Realtime Database** -> bấm **Create Database**.
   - Chọn khu vực lưu trữ, bấm **Next**.
   - Chọn **Start in test mode** (để cấu hình Rules sau này), bấm **Enable**.
   - Lưu lại địa chỉ **Database URL** (ví dụ: `https://lapguard-default-rtdb.firebaseio.com/`).
5. **Đăng ký Web App để lấy Config**:
   - Về lại trang chủ dự án (Project Overview), nhấp vào biểu tượng **Web (`</>`)**.
   - Nhập tên ứng dụng (ví dụ: `LapGuard Web UI`), nhấp **Register app**.
   - Sao chép lại đối tượng `firebaseConfig` được hiển thị. Bác sẽ cần các thông số `apiKey` và `databaseURL` để nạp vào ESP32 và React Web App.

### A.2 Thiết lập React Web App

1. Tải mã nguồn của React Web App về máy tính.
2. Tạo file `.env` từ file mẫu `.env.example`:
   ```bash
   cp .env.example .env
   ```
3. Mở file `.env` và điền đầy đủ các thông số cấu hình Firebase bác vừa lấy ở mục A.1:
   ```env
   VITE_FIREBASE_API_KEY="AIzaSy..."
   VITE_FIREBASE_AUTH_DOMAIN="lapguard-smart-alarm.firebaseapp.com"
   VITE_FIREBASE_DATABASE_URL="https://lapguard-default-rtdb.firebaseio.com"
   VITE_FIREBASE_PROJECT_ID="lapguard-smart-alarm"
   VITE_FIREBASE_STORAGE_BUCKET="lapguard-smart-alarm.appspot.com"
   VITE_FIREBASE_MESSAGING_SENDER_ID="123456789"
   VITE_FIREBASE_APP_ID="1:1234..."
   VITE_FIREBASE_VAPID_KEY="..."
   ```
4. Tạo Web Push key cho FCM:
   - Firebase Console -> Project settings -> Cloud Messaging.
   - Ở mục **Web Push certificates**, bấm **Generate key pair**.
   - Copy public key vào `VITE_FIREBASE_VAPID_KEY`.
5. Chạy cài đặt và khởi động React Web App ở chế độ chạy thử:
   ```bash
   npm install
   npm run dev
   ```
6. Deploy Firebase Hosting sau khi build/test pass (không cần Blaze plan):
   ```bash
   npm run build
   cd ..
   npx firebase-tools deploy --only hosting --project aiot-929e6
   ```

### A.3 Chạy Push Server để gửi FCM Web Push

Firebase Cloud Functions yêu cầu Blaze plan. Để tránh cần nâng cấp plan, dự án dùng một server Node.js riêng trong thư mục `push-server/`.

1. Tạo service account key trong Firebase Console:
   - Project settings -> Service accounts.
   - Bấm **Generate new private key**.
   - Lưu file JSON thành `push-server/service-account.json`.
2. Tạo file `.env`:
   ```bash
   cd push-server
   cp .env.example .env
   ```
3. Điền:
   ```env
   FIREBASE_DATABASE_URL="https://aiot-929e6-default-rtdb.asia-southeast1.firebasedatabase.app"
   GOOGLE_APPLICATION_CREDENTIALS="./service-account.json"
   PORT=3001
   ```
4. Chạy server:
   ```bash
   npm install
   npm start
   ```

Server này lắng nghe `/logs` và gửi FCM push tới token của chủ thiết bị. Nếu server không chạy, Web App vẫn có cảnh báo realtime khi tab đang mở, nhưng sẽ không có push khi app đã đóng.

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
   - Nối thêm INT -> GPIO 13 nếu muốn dùng hardware interrupt sau này.

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
#define FIREBASE_API_KEY      "AIzaSyA1xxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define FIREBASE_DATABASE_URL "https://lapguard-default-rtdb.firebaseio.com"

#define WIFI_AP_SSID          "LapGuard_AP"
#define WIFI_AP_PASSWORD      "12345678" // Mật khẩu phát ra để cấu hình

#define DEVICE_NAME           "LapGuard-01"
```

> Đảm bảo `secrets.h` đã được thêm vào `.gitignore` để không lộ các khóa bảo mật lên GitHub.

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

Log dự kiến (khi đã kết nối WiFi thành công):

```
[BOOT] LapGuard v1.0.0 starting...
[BOOT] Chip: ESP32, cores: 2, flash: 4MB
[WIFI] WiFiManager: Attempting to connect...
[WIFI] Connected, IP: 192.168.1.42, RSSI: -52 dBm
[I2C]  Scanning... found 0x68 (MPU6500)
[MPU]  Init OK
[FB]   Firebase Client initialized
[FB]   Listening to command node: /devices/240AC4123456/command
[FSM]  State -> DISARMED
[BOOT] Ready.
```

*Lưu ý*: Nếu thiết bị bật lên lần đầu hoặc không thể kết nối WiFi cũ, log sẽ báo:
`[WIFI] WiFiManager: AP Mode active. SSID: LapGuard_AP. IP: 192.168.4.1`
Người dùng cần dùng điện thoại bắt WiFi `LapGuard_AP` và nhập mật khẩu `12345678` để cấu hình mạng.

## 6. Phần E - Kiểm thử lần đầu

### E.1 Test Đăng nhập và Liên kết thiết bị trên Web App

1. Mở React Web App trên trình duyệt điện thoại.
2. Đăng ký tài khoản bằng Email/Mật khẩu bất kỳ, sau đó tiến hành Đăng nhập.
3. Nhấp nút **Thêm thiết bị mới**, nhập tên thiết bị và địa chỉ MAC của ESP32 (lấy từ log Serial khi thiết bị khởi động).
4. Xác nhận trạng thái thiết bị hiển thị trên màn hình Dashboard là **Online** và trạng thái là **DISARMED**.

### E.2 Test bật/tắt báo động thời gian thực

1. Trên màn hình Dashboard, nhấp nút **ARM**.
2. Xác nhận đèn LED xanh trên thiết bị chuyển sang nhấp nháy đỏ chậm, trạng thái hiển thị trên App cập nhật tức thời thành **ARMED**.
3. Nhấp nút **DISARM** trên App.
4. Xác nhận LED đỏ tắt, LED xanh sáng liên tục, trạng thái cập nhật thành **DISARMED**.

### E.3 Test phát hiện chuyển động và nẩy thông báo đẩy (Web Push)

1. Nhấn nút **ARM** trên Web App để kích hoạt bảo vệ.
2. Nhấc nhẹ thiết bị lên khoảng 5cm.
3. Xác nhận:
   - Còi hú tại chỗ vang dội, LED đỏ sáng liên tục.
   - Trạng thái trên App chuyển ngay thành **TRIGGERED**.
   - Điện thoại nhận được một thông báo đẩy cảnh báo có trộm từ Web App.
4. Nhấn nút **DISARM** trên Web App để tắt còi hú.

### E.4 Tinh chỉnh ngưỡng cảm biến

Nếu còi hú quá nhạy (báo động giả khi gõ nhẹ bàn):
- Tăng giá trị `MOTION_THRESHOLD_G` trong [config.h](file:///C:/Users/cuphu/OneDrive/M%C3%A1y%20t%C3%ADnh/AIoT/firmware/src/config.h) (ví dụ từ 0.3 -> 0.4 hoặc 0.5).
- Hoặc tăng số mẫu tích lũy lọc nhiễu `MOTION_PERSISTENCE_N` (ví dụ từ 3 -> 5 mẫu).

Nếu cảm biến quá lỳ (nhấc hẳn laptop đi mà không hú còi):
- Giảm nhẹ `MOTION_THRESHOLD_G`.

### E.5 Test chế độ offline (mất WiFi)

1. Bật chế độ bảo vệ (**ARM**).
2. Tắt router WiFi (hoặc ngắt nguồn WiFi cấp cho ESP32).
3. Xác định thiết bị chuyển LED sang nháy cam (offline), trạng thái trên Web App báo **Offline**.
4. Rung lắc mạnh thiết bị: Xác nhận còi hú vẫn vang lên tại chỗ (bảo vệ local vẫn hoạt động).
5. Bật lại router WiFi: Xác nhận ESP32 tự động kết nối lại mạng trong 10-20 giây, đẩy nhật ký sự kiện trộm lên Firebase và hiển thị chi tiết lịch sử trộm trên màn hình Web App của bạn.


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
3. Mở Web App trên điện thoại để xác nhận thiết bị online, pin và sóng hoạt động ổn định.
4. Khi rời chỗ ngồi: Nhấn nút **ARM** trên màn hình Web App.
5. Khi quay lại: Nhấn nút **DISARM** trên màn hình Web App trước khi chạm tay vào laptop.

## 8. Troubleshooting

### 8.1 ESP32 không nhận trên máy tính

- Cắm cáp USB Micro-B khác (đảm bảo cáp có dây data, không phải cáp chỉ sạc).
- Cài đúng driver CP210x hoặc CH340.
- Kiểm tra Device Manager có COM port không.

### 8.2 Thiết bị không đồng bộ lệnh từ Web App (Gặp lỗi Firebase)

- Kiểm tra Serial Monitor xem kết nối Firebase có bị báo lỗi không (ví dụ: lỗi bắt tay SSL, sai API Key hoặc Database URL).
- Kiểm tra lại phân quyền **Database Rules** trên Firebase Console xem có chặn quyền ghi của ứng dụng/thiết bị không.
- Kiểm tra xem thiết bị đã được liên kết đúng với mã MAC tương ứng của tài khoản hay chưa.

### 8.3 ESP32 reset liên tục khi bật WiFi

- Nguồn yếu. Đo điện áp VIN khi WiFi truyền phát dữ liệu, nếu tụt dưới 4.5V -> nguồn không đủ dòng.
- Thay module tăng áp khác, hoặc cắm nguồn qua cáp USB máy tính để loại trừ khả năng chai pin 18650.
- Thêm tụ 470uF ở VIN để ổn định dòng.

### 8.4 Cảm biến MPU6050/6500 không hoạt động

- Kiểm tra dây SDA, SCL xem có cắm đảo chiều hay lỏng chân không.
- Đo thông mạch các đường dây từ cảm biến về ESP32.
- Kiểm tra xem địa chỉ I2C của module là `0x68` hay `0x69`. Nếu sensor trả về `WHO_AM_I = 0x70` tức là chip MPU6500, driver tự viết của chúng ta đã hỗ trợ hoàn toàn tự động nhận diện cả hai chip.

### 8.5 Còi không kêu

- Đo điện áp chân điều khiển buzzer khi kích hoạt báo động xem có xuất tín hiệu 3.3V ra GPIO 25 không.
- Kiểm tra cực tính của còi hú (+/-).

### 8.6 Điện thoại không nhận được thông báo đẩy (Web Push) khi có trộm

- Đảm bảo bạn đã cấp quyền cho phép trình duyệt gửi thông báo khi trang web hiển thị popup xin quyền.
- Với iPhone (iOS): Nhấp nút Share -> chọn **Add to Home screen**, sau đó khởi chạy ứng dụng LapGuard từ icon trên màn hình chính và cấp quyền thông báo đẩy.

### 8.7 False positive khi laptop đứng yên

- Tăng threshold (ngưỡng rung).
- Kéo dài persistence (số mẫu rung liên tiếp cần đạt).
- Kiểm tra vỏ có rung theo quạt laptop không và dán chặt lại thiết bị.

## 9. Checklist bàn giao

Dùng checklist này khi nộp bài / demo cho giảng viên:

- [x] Thiết bị khởi động nhanh, tự động mở Captive Portal cấu hình WiFi nếu chưa có mạng.
- [x] Dashboard hiển thị chính xác trạng thái Online/Offline, % pin và sóng RSSI của thiết bị.
- [x] Nhấn nút **ARM** trên Web App $\rightarrow$ Thiết bị chuyển sang giám sát trong <1 giây.
- [x] Nhấc thiết bị $\rightarrow$ Còi hú vang lập tức và Web App nhận thông báo đẩy trong <3 giây.
- [x] Nhấn nút **DISARM** trên Web App $\rightarrow$ Còi tắt lập tức và chuyển về trạng thái an toàn.
- [x] Ngắt WiFi $\rightarrow$ Thiết bị báo offline trên Web App, còi hú tại chỗ vẫn hoạt động bình thường khi dịch chuyển.
- [x] Có lại WiFi $\rightarrow$ Thiết bị tự động reconnect và đồng bộ đầy đủ nhật ký báo động trong lúc offline lên Web App.
- [x] Chạy pin liên tục >= 8 giờ.
- [x] Vỏ hộp đóng gọn, có nhãn dán tên nhóm.
- [x] Slide thuyết trình + video demo sẵn sàng.
- [x] Mã nguồn đã push lên GitHub, không chứa `secrets.h`.
