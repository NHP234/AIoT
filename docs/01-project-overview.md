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
- **Thông báo tức thời** đến điện thoại chủ sở hữu dưới dạng thông báo đẩy (Web Push) dù đang ở xa thiết bị.
- Cho phép chủ **vô hiệu hoá tạm thời** từ xa thông qua ứng dụng Web React.
- Hoạt động thông qua **ứng dụng Web React (PWA)** trực quan và tiện lợi trên điện thoại hoặc máy tính.

## 3. Mục tiêu dự án

### Mục tiêu chính

1. Xây dựng thiết bị phần cứng hoàn chỉnh dựa trên ESP32, phát hiện chuyển động trong
   **dưới 200 ms** kể từ khi bắt đầu tác động.
2. Gửi thông báo đẩy (Web Push) đến điện thoại với độ trễ **dưới 3 giây** tính từ lúc trigger.
3. Còi báo động có âm lượng **>= 85 dB ở 10 cm** để đủ gây chú ý.
4. Pin 18650 đơn đủ cho thiết bị hoạt động **>= 8 giờ** ở chế độ giám sát liên tục.
5. Tất cả tính năng arm / disarm / silence đều điều khiển trực quan được qua Web App React thời gian thực.

### Mục tiêu học thuật

- Thực hành đầy đủ chu trình một dự án IoT end-to-end: **cảm biến -> MCU -> mạng -> cloud (Firebase) -> ứng dụng người dùng (React Web App)**.
- Vận dụng các khái niệm: I2C, ngắt GPIO, WiFi, WebSockets, Firebase API, NVS, FSM, Web Push (FCM), Captive Portal (WiFiManager).
- Rèn kỹ năng làm tài liệu, viết test case, quản lý tiến độ theo tuần.

## 4. Phạm vi (Scope)

### Trong phạm vi (In-Scope)

- Phát hiện chuyển động tịnh tiến và nghiêng qua MPU6050/MPU6500.
- Còi báo động cục bộ.
- LED hiển thị trạng thái (offline / disarmed / armed / triggered).
- Gửi thông báo đẩy (Web Push) qua Firebase Cloud Messaging khi trigger, khi pin yếu.
- Đăng nhập, đăng ký tài khoản và quản lý liên kết thiết bị theo địa chỉ MAC.
- Điều khiển Arm / disarm / silence / xem trạng thái pin, sóng mạng trực quan qua React Web App.
- Cấu hình WiFi cho thiết bị tiện lợi qua trang Captive Portal (WiFiManager) tự động phát sóng.
- Nguồn pin 18650 sạc qua USB-C.

### Ngoài phạm vi (Out-of-Scope, có thể làm ở phiên bản sau)

- Định vị GPS khi vật dụng đã bị mang đi.
- Camera ESP32-CAM chụp ảnh kẻ trộm.
- Nhận diện khuôn mặt / vân tay để disarm.
- Khoá vật lý / khoá từ.
- App Native phân phối trên App Store / CH Play (hiện tại dùng Web App PWA cài nhanh).
- Giao tiếp LoRa / Zigbee / 4G khi không có WiFi.
- Tích hợp cảm biến rung chuyên dụng SW-420.

## 5. Đối tượng người dùng

- **Sinh viên** mang laptop tới trường, thư viện, quán cà phê.
- **Giảng viên / nhân viên văn phòng** thỉnh thoảng rời bàn làm việc.
- **Khách du lịch** làm việc tại coworking space, sân bay.

Yêu cầu tối thiểu của người dùng: có smartphone kết nối Internet và có WiFi local ở nơi đặt thiết bị.

## 6. Yêu cầu chức năng (Functional Requirements)

| ID | Mô tả | Ưu tiên |
|----|-------|---------|
| FR1 | Thiết bị phát hiện chuyển động khi `|gia_tốc| - 1g > THRESHOLD` kéo dài qua N mẫu liên tiếp | Must |
| FR2 | Thiết bị phát hiện rung tức thời qua cảm biến rung SW-420 | Defer (Hẹn phiên bản sau) |
| FR3 | Khi trigger, bật còi trong chế độ hú liên tục tối đa 60 giây | Must |
| FR4 | Khi trigger, gửi thông báo đẩy (Web Push) lên màn hình điện thoại người dùng | Must |
| FR5 | Người dùng nhấn nút ARM trên Web App để chuyển sang trạng thái giám sát | Must |
| FR6 | Người dùng nhấn nút DISARM trên Web App để tắt còi và ngừng giám sát | Must |
| FR7 | Người dùng nhấn nút SILENCE trên Web App để tắt còi nhưng vẫn giữ trạng thái giám sát | Should |
| FR8 | Người dùng xem trạng thái thực tế, mức pin %, RSSI WiFi thời gian thực trên Web App | Should |
| FR9 | Người dùng đăng ký/đăng nhập tài khoản bảo mật bằng Email/Mật khẩu trên Web App | Must |
| FR10 | Người dùng có thể liên kết thiết bị mới vào tài khoản của mình bằng địa chỉ MAC | Must |
| FR11 | ESP32 tự động mở Captive Portal cấu hình WiFi khi không kết nối được WiFi cũ | Must |
| FR12 | Cảnh báo pin yếu khi điện áp < 3.4 V | Could |
| FR13 | Tự động reconnect WiFi khi mất kết nối, còi vẫn hoạt động cục bộ | Must |
| FR14 | Khi có lại WiFi sau mất kết nối, đồng bộ và hiển thị nhật ký các sự kiện đã xảy ra trong lúc offline lên Web App | Should |

## 7. Yêu cầu phi chức năng (Non-Functional Requirements)

| ID | Nhóm | Mô tả | Chỉ tiêu |
|----|------|-------|----------|
| NFR1 | Hiệu năng | Độ trễ từ trigger đến khi còi kêu | < 200 ms |
| NFR2 | Hiệu năng | Độ trễ từ trigger đến khi Web App nhận được cảnh báo / Push Notification | < 3 s |
| NFR3 | Hiệu năng | Tỉ lệ báo động giả (false positive) khi vật dụng đứng yên | < 1 lần / ngày |
| NFR4 | Độ tin cậy | Thiết bị tự phục hồi sau mất điện / reset mềm | Có watchdog |
| NFR5 | Năng lượng | Thời lượng pin ở chế độ giám sát liên tục | >= 8 giờ |
| NFR6 | Kích thước | Kích thước vỏ hộp | <= 7 x 5 x 3 cm |
| NFR7 | Âm thanh | Âm lượng còi | >= 85 dB @ 10 cm |
| NFR8 | Bảo mật | Dữ liệu đồng bộ và điều khiển trên Firebase được bảo mật bởi Rules (chỉ chủ sở hữu được ghi lệnh) | Đạt chuẩn Firebase |
| NFR9 | Bảo mật | Thông tin nhạy cảm (Firebase API Key, Credentials) không commit vào git | File `secrets.h` trong `.gitignore` |
| NFR10 | Chi phí | Tổng chi phí linh kiện | <= 400.000 VND |
| NFR11 | Bảo trì | Code firmware có comment, chia module rõ ràng | >= 6 module |
| NFR12 | Khả năng dùng | Thời gian setup WiFi lần đầu qua Captive Portal | <= 5 phút |

## 8. Kịch bản sử dụng tiêu biểu

### Kịch bản 1: Sinh viên đi vệ sinh ở thư viện

1. An đặt laptop trên bàn, gắn LapGuard phía dưới.
2. An mở Web App trên điện thoại, nhấn nút **ARM**. LED chuyển xanh -> đỏ nhấp nháy (ARMED).
3. An rời bàn đi vệ sinh.
4. Một người lạ tới và nhấc laptop lên xem.
5. Cảm biến phát hiện thay đổi gia tốc > ngưỡng, ESP32 chuyển sang TRIGGERED.
6. Còi kêu hú chói tai. Điện thoại của An nhận được thông báo đẩy (Web Push) thời gian thực:
   > CẢNH BÁO LAPGUARD: Phát hiện chuyển động mạnh (delta = 3.2g) trên thiết bị LapGuard Laptop của An!
7. Kẻ lạ hoảng sợ đặt lại laptop và bỏ đi.
8. An chạy lại, mở Web App nhấn **DISARM**, còi tắt.

### Kịch bản 2: Chủ nhân tự di chuyển laptop hợp pháp

1. Bình muốn thu dọn laptop để về nhà.
2. Bình mở Web App nhấn **DISARM** trước khi chạm vào laptop.
3. Thiết bị chuyển về DISARMED, LED xanh liên tục.
4. Bình thu dọn bình thường, không báo động.

### Kịch bản 3: Mất WiFi tạm thời

1. Thiết bị đang ARMED thì router WiFi của thư viện bị ngắt.
2. ESP32 chuyển LED sang chế độ chớp cam (mất kết nối).
3. Nếu có trigger, **còi vẫn kêu cục bộ bình thường**, sự kiện được lưu trong RAM.
4. Khi WiFi có lại, thiết bị tự reconnect với Firebase và đồng bộ nhật ký sự kiện offline lên Web App cho người dùng xem.

## 9. Tiêu chí thành công

Dự án được coi là thành công nếu đạt được đầy đủ các điểm sau trong buổi báo cáo:

- [x] Demo trực tiếp cho thầy cô 3 kịch bản ở mục 8 chạy ổn định.
- [x] Hoàn thành >= 90% FR ở mức "Must".
- [x] Đạt tất cả NFR ở nhóm Hiệu năng và Bảo mật.
- [x] Bộ tài liệu gồm 7 file markdown đầy đủ, có sơ đồ minh hoạ.
- [x] Video demo <= 2 phút được upload công khai.
- [x] Báo cáo powerpoint / pdf trình bày kiến trúc và kết quả.
- [x] Chi phí thực tế <= 400.000 VND.
