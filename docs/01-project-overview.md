# 01 - Tổng quan dự án

## Mục lục

- [1. Bối cảnh](#1-bối-cảnh)
- [2. Vấn đề cần giải quyết](#2-vấn-đề-cần-giải-quyết)
- [3. Mục tiêu dự án](#3-mục-tiêu-dự-án)
- [4. Phạm vi (Scope)](#4-phạm-vi-scope)
- [5. Đối tượng người dùng](#5-đối-tượng-người-dùng)
- [6. Yêu cầu chức năng (Functional Requirements)](#6-yêu-cầu-chức-năng-functional-requirements)
- [7. Yêu cầu phi chức năng (Non-Functional Requirements)](#7-yêu-cầu-phi-chức-năng-non-functional-requirements)
- [8. Kịch bản sử dụng tiêu biểu](#8-kịch-bản-sử-dụng-tiêu-biểu)
- [9. Tiêu chí thành công](#9-tiêu-chí-thành-công)

---

## 1. Bối cảnh

Laptop là tài sản có giá trị cao và rất phổ biến với sinh viên. Tuy nhiên, trong môi trường
học đường (thư viện, căng-tin, phòng tự học, quán cà phê gần trường), sinh viên thường phải
rời khỏi chỗ ngồi trong thời gian ngắn (đi vệ sinh, lấy đồ uống, in tài liệu). Khoảnh khắc
vắng mặt 1-2 phút là cơ hội lý tưởng cho kẻ gian.

Các giải pháp hiện tại có nhiều hạn chế:

- **Khoá dây cáp Kensington**: giá cao, không tiện mang theo, nhiều laptop không có khe khoá.
- **Nhờ người ngồi cạnh trông giúp**: không đáng tin, không phải lúc nào cũng có.
- **Phần mềm Find My / Prey**: chỉ định vị *sau khi* bị mất, không ngăn chặn được tại thời điểm trộm.

Dự án LapGuard ra đời để lấp khoảng trống này: một thiết bị **giá rẻ (~300-400k VND), nhỏ gọn,
chủ động cảnh báo ngay khoảnh khắc có kẻ đụng vào laptop**.

## 2. Vấn đề cần giải quyết

- Phát hiện hành vi **di chuyển / nhấc / va đập** laptop ngay khi nó vừa xảy ra.
- Tạo tiếng ồn đủ lớn để **xua đuổi kẻ gian và cảnh báo người xung quanh**.
- **Thông báo tức thời** đến điện thoại chủ sở hữu dù đang ở xa thiết bị.
- Cho phép chủ **vô hiệu hoá tạm thời** khi cần tự di chuyển laptop.
- Hoạt động **không cần ứng dụng riêng** trên điện thoại (tận dụng Telegram có sẵn).

## 3. Mục tiêu dự án

### Mục tiêu chính

1. Xây dựng thiết bị phần cứng hoàn chỉnh dựa trên ESP32, phát hiện chuyển động trong
   **dưới 200 ms** kể từ khi bắt đầu tác động.
2. Gửi thông báo đến Telegram với độ trễ **dưới 3 giây** tính từ lúc trigger.
3. Còi báo động có âm lượng **>= 85 dB ở 10 cm** để đủ gây chú ý.
4. Pin 18650 đơn đủ cho thiết bị hoạt động **>= 8 giờ** ở chế độ giám sát liên tục.
5. Tất cả tính năng arm / disarm / silence đều điều khiển được qua Telegram có PIN bảo mật.

### Mục tiêu học thuật

- Thực hành đầy đủ chu trình một dự án IoT end-to-end: **cảm biến -> MCU -> mạng -> cloud -> ứng dụng người dùng**.
- Vận dụng các khái niệm: I2C, ngắt GPIO, WiFi, HTTPS, REST API, NVS, FSM, hash mật khẩu.
- Rèn kỹ năng làm tài liệu, viết test case, quản lý tiến độ theo tuần.

## 4. Phạm vi (Scope)

### Trong phạm vi (In-Scope)

- Phát hiện chuyển động tịnh tiến và nghiêng qua MPU6050.
- Phát hiện va đập rung qua SW-420.
- Còi báo động cục bộ.
- LED hiển thị trạng thái (offline / disarmed / armed / triggered).
- Gửi tin nhắn Telegram khi trigger, khi mất/có lại WiFi, khi pin yếu.
- Arm / disarm / silence / đổi PIN / xem trạng thái qua Telegram.
- Nguồn pin 18650 sạc qua USB-C.

### Ngoài phạm vi (Out-of-Scope, có thể làm ở phiên bản sau)

- Định vị GPS khi laptop đã bị mang đi.
- Camera ESP32-CAM chụp ảnh kẻ trộm.
- Nhận diện khuôn mặt / vân tay để disarm.
- Khoá vật lý / khoá từ.
- Ứng dụng mobile riêng (Android / iOS).
- Giao tiếp LoRa / Zigbee / 4G khi không có WiFi.

## 5. Đối tượng người dùng

- **Sinh viên** mang laptop tới trường, thư viện, quán cà phê.
- **Giảng viên / nhân viên văn phòng** thỉnh thoảng rời bàn làm việc.
- **Khách du lịch** làm việc tại coworking space, sân bay.

Yêu cầu tối thiểu của người dùng: có smartphone cài Telegram và có WiFi ở nơi đặt thiết bị.

## 6. Yêu cầu chức năng (Functional Requirements)

| ID | Mô tả | Ưu tiên |
|----|-------|---------|
| FR1 | Thiết bị phát hiện chuyển động khi `|gia_tốc| - 1g > THRESHOLD` kéo dài qua N mẫu liên tiếp | Must |
| FR2 | Thiết bị phát hiện rung tức thời qua SW-420 (ngắt GPIO) | Must |
| FR3 | Khi trigger, bật còi trong chế độ hú liên tục tối đa 60 giây | Must |
| FR4 | Khi trigger, gửi tin nhắn Telegram kèm timestamp và mức độ rung đo được | Must |
| FR5 | Người dùng gửi `/arm <PIN>` để chuyển sang trạng thái giám sát | Must |
| FR6 | Người dùng gửi `/disarm <PIN>` để tắt còi và ngừng giám sát | Must |
| FR7 | Người dùng gửi `/silence <PIN>` để tắt còi nhưng vẫn giữ trạng thái giám sát | Should |
| FR8 | Người dùng gửi `/status` để xem trạng thái hiện tại, RSSI WiFi, mức pin | Should |
| FR9 | Người dùng gửi `/setpin <old> <new>` để đổi PIN | Should |
| FR10 | Nhập sai PIN 3 lần liên tiếp sẽ khoá lệnh trong 30 giây | Must |
| FR11 | Chỉ chấp nhận lệnh từ danh sách `chat_id` được whitelist | Must |
| FR12 | Cảnh báo pin yếu khi điện áp < 3.4 V | Could |
| FR13 | Tự động reconnect WiFi khi mất kết nối, còi vẫn hoạt động cục bộ | Must |
| FR14 | Khi có lại WiFi sau mất kết nối, gửi tin nhắn tóm tắt các sự kiện đã xảy ra | Should |

## 7. Yêu cầu phi chức năng (Non-Functional Requirements)

| ID | Nhóm | Mô tả | Chỉ tiêu |
|----|------|-------|----------|
| NFR1 | Hiệu năng | Độ trễ từ trigger đến khi còi kêu | < 200 ms |
| NFR2 | Hiệu năng | Độ trễ từ trigger đến khi Telegram nhận được tin nhắn | < 3 s |
| NFR3 | Hiệu năng | Tỉ lệ báo động giả (false positive) khi laptop đứng yên | < 1 lần / ngày |
| NFR4 | Độ tin cậy | Thiết bị tự phục hồi sau mất điện / reset mềm | Có watchdog |
| NFR5 | Năng lượng | Thời lượng pin ở chế độ giám sát liên tục | >= 8 giờ |
| NFR6 | Kích thước | Kích thước vỏ hộp | <= 7 x 5 x 3 cm |
| NFR7 | Âm thanh | Âm lượng còi | >= 85 dB @ 10 cm |
| NFR8 | Bảo mật | PIN không lưu dạng plaintext | Hash SHA-256 + salt |
| NFR9 | Bảo mật | Thông tin nhạy cảm (token, SSID) không commit vào git | File `secrets.h` trong `.gitignore` |
| NFR10 | Chi phí | Tổng chi phí linh kiện | <= 400.000 VND |
| NFR11 | Bảo trì | Code firmware có comment, chia module rõ ràng | >= 6 module |
| NFR12 | Khả năng dùng | Thời gian setup lần đầu cho người mới | <= 15 phút |

## 8. Kịch bản sử dụng tiêu biểu

### Kịch bản 1: Sinh viên đi vệ sinh ở thư viện

1. An đặt laptop trên bàn, gắn LapGuard phía dưới.
2. An mở Telegram, gửi `/arm 1234` cho bot. LED chuyển xanh -> đỏ nhấp nháy (ARMED).
3. An rời bàn đi vệ sinh.
4. Một người lạ tới và nhấc laptop lên xem.
5. MPU6050 phát hiện thay đổi gia tốc > ngưỡng, ESP32 chuyển sang TRIGGERED.
6. Còi kêu hú chói tai. Telegram của An reo với nội dung:
   > LapGuard ALERT! Phat hien chuyen dong manh (delta = 3.2g) luc 14:23:17. Da bat coi bao dong.
7. Kẻ lạ hoảng sợ đặt lại laptop và bỏ đi.
8. An chạy lại, gửi `/disarm 1234`, còi tắt.

### Kịch bản 2: Chủ nhân tự di chuyển laptop hợp pháp

1. Bình muốn thu dọn laptop để về nhà.
2. Bình gửi `/disarm 1234` trước khi chạm vào laptop.
3. Thiết bị chuyển về DISARMED, LED xanh liên tục.
4. Bình thu dọn bình thường, không báo động.

### Kịch bản 3: Mất WiFi tạm thời

1. Thiết bị đang ARMED thì router WiFi của thư viện bị ngắt.
2. ESP32 chuyển LED sang chế độ chớp cam (mất kết nối).
3. Nếu có trigger, **còi vẫn kêu bình thường**, sự kiện được lưu trong RAM.
4. Khi WiFi có lại, thiết bị tự reconnect và gửi tóm tắt các sự kiện đã xảy ra trong thời gian offline.

## 9. Tiêu chí thành công

Dự án được coi là thành công nếu đạt được đầy đủ các điểm sau trong buổi báo cáo:

- [ ] Demo trực tiếp cho thầy cô 3 kịch bản ở mục 8 chạy ổn định.
- [ ] Hoàn thành >= 90% FR ở mức "Must".
- [ ] Đạt tất cả NFR ở nhóm Hiệu năng và Bảo mật.
- [ ] Bộ tài liệu gồm 7 file markdown đầy đủ, có sơ đồ minh hoạ.
- [ ] Video demo <= 2 phút được upload công khai.
- [ ] Báo cáo powerpoint / pdf trình bày kiến trúc và kết quả.
- [ ] Chi phí thực tế <= 400.000 VND.
