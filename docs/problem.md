# Báo cáo xử lý lỗi ESP32 không nhận cảm biến

## 1. Hiện tượng

ESP32 khởi động bình thường nhưng firmware báo:

```text
[MOTION] MPU6050 init failed
```

Người dùng đã hàn chân cảm biến và nối dây theo cấu hình:

| Cảm biến | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO 21 (D21) |
| SCL | GPIO 22 (D22) |
| AD0 | GND |

Mục tiêu là xác định lỗi thuộc firmware, dây nối, nguồn hay bản thân cảm biến.

## 2. Quy trình chẩn đoán

### 2.1 Kiểm tra ESP32 và firmware hiện tại

ESP32 được máy tính nhận tại `COM5` qua chip USB-UART CH340. Log khởi động cho
thấy ESP32 chạy ổn định, kết nối được WiFi và không có dấu hiệu brown-out hoặc
reset liên tục.

Firmware build thành công và các unit test của bộ lọc chuyển động đều đạt. Tuy
nhiên, các test này chỉ kiểm tra thuật toán, không kiểm tra cảm biến thật.

### 2.2 Đọc log khởi tạo cảm biến

Log thực tế:

```text
[I2C] Initialising I2C Master: sda=21 scl=22 freq=100000
[MOTION] MPU6050 init failed
```

Lỗi xuất hiện tại bước khởi tạo driver, trước bước tính toán chuyển động. Vì vậy
ngưỡng phát hiện và bộ lọc chuyển động không phải nguyên nhân.

### 2.3 Quét bus I2C

Một firmware chẩn đoán tạm thời được nạp để quét bus với hai cấu hình:

1. SDA = GPIO 21, SCL = GPIO 22.
2. SDA = GPIO 22, SCL = GPIO 21 để kiểm tra trường hợp nối đảo.

Kết quả:

```text
[SCAN] SDA=GPIO21, SCL=GPIO22
[SCAN] Found I2C device at 0x68

[SCAN] SDA=GPIO22, SCL=GPIO21
[SCAN] No I2C devices found
```

Kết quả này xác nhận:

- Cảm biến có nguồn và phản hồi trên bus I2C.
- SDA và SCL đang được nối đúng.
- AD0 nối GND đúng, nên địa chỉ I2C là `0x68`.
- Mối hàn và dây nối đủ tốt để truyền dữ liệu.

### 2.4 Đọc thanh ghi định danh

Thanh ghi `WHO_AM_I` tại địa chỉ `0x75` được đọc trực tiếp:

```text
[ID] Address 0x68 WHO_AM_I=0x70
```

Theo tài liệu chip:

- MPU6050 trả về `WHO_AM_I = 0x68`.
- MPU6500 trả về `WHO_AM_I = 0x70`.

Như vậy module được bán hoặc ghi nhãn như MPU6050 nhưng chip thực tế trên module
là MPU6500.

### 2.5 Kiểm tra dữ liệu gia tốc

Firmware chẩn đoán cấu hình MPU6500 ở dải `+-4g` và đọc dữ liệu gia tốc thô:

```text
[DATA] ax=-0.525g ay=-0.118g az=+0.909g
[DATA] ax=-0.523g ay=-0.117g az=+0.908g
[DATA] ax=-0.524g ay=-0.120g az=+0.906g
```

Độ lớn vector gia tốc xấp xỉ `1g` khi cảm biến đứng yên. Điều này xác nhận chip
không chỉ phản hồi địa chỉ mà còn đo được dữ liệu hợp lý.

## 3. Nguyên nhân gốc

Nguyên nhân là **firmware không tương thích với chip thực tế**, không phải lỗi
dây nối hoặc phần cứng.

Firmware cũ sử dụng thư viện `Adafruit_MPU6050`. Trong hàm khởi tạo, thư viện
đọc `WHO_AM_I` và chỉ chấp nhận giá trị của MPU6050 là `0x68`.

Cảm biến thực tế là MPU6500, trả về `0x70`, nên thư viện chủ động trả về thất
bại dù thiết bị vẫn hoạt động bình thường tại địa chỉ I2C `0x68`.

Lưu ý: địa chỉ I2C và giá trị `WHO_AM_I` là hai thông tin khác nhau:

- Địa chỉ giao tiếp I2C: `0x68` hoặc `0x69`, phụ thuộc chân AD0.
- ID loại chip: MPU6050 là `0x68`, MPU6500 là `0x70`.

## 4. Thay đổi firmware

Module cảm biến trong `firmware/src/motion/motion.cpp` được sửa như sau:

1. Bỏ phụ thuộc vào driver chỉ hỗ trợ MPU6050.
2. Tự quét cả hai địa chỉ I2C `0x68` và `0x69`.
3. Đọc thanh ghi `WHO_AM_I`.
4. Chấp nhận:
   - `0x68`: MPU6050.
   - `0x70`: MPU6500.
5. Cấu hình trực tiếp các thanh ghi dùng chung:
   - Clock source.
   - Sample rate.
   - DLPF.
   - Gyroscope `+-500 deg/s`.
   - Accelerometer `+-4g`.
6. Cấu hình thêm thanh ghi lọc gia tốc riêng của MPU6500.
7. Đọc trực tiếp khối 14 byte từ thanh ghi `0x3B`, gồm gia tốc, nhiệt độ và gyro.
8. Chuyển dữ liệu thô sang đơn vị `g` với hệ số `8192 LSB/g`.
9. Giữ nguyên bộ lọc và thuật toán phát hiện chuyển động của dự án.
10. Thêm log hiển thị loại chip, địa chỉ và kết quả gửi Telegram.

Hai thư viện không còn cần thiết đã được bỏ khỏi `platformio.ini`:

```text
Adafruit MPU6050
Adafruit Unified Sensor
```

Tài liệu phần cứng, thiết kế firmware và troubleshooting cũng được cập nhật để
ghi nhận trường hợp module MPU6050 thực tế gắn MPU6500.

## 5. Kết quả xác minh

Sau khi build và nạp firmware mới, Serial log hiển thị:

```text
[MOTION] MPU6500 initialized at 0x68 (WHO_AM_I=0x70)
```

Khi gửi lệnh `/arm <PIN>` và di chuyển cảm biến:

```text
[FSM] ARMED -> TRIGGERED
```

Telegram nhận được cảnh báo gồm:

- Motion delta.
- State `TRIGGERED`.
- RSSI WiFi.

Firmware ESP32 build thành công và 4 unit test hiện có đều đạt.

## 6. Kết luận

| Hạng mục | Kết quả |
|---|---|
| Nguồn ESP32 | Bình thường |
| Cáp USB và driver | Bình thường |
| Dây SDA/SCL | Nối đúng |
| Chân AD0 | Đúng, địa chỉ `0x68` |
| Mối hàn và bus I2C | Hoạt động |
| Chip cảm biến | MPU6500, hoạt động |
| Firmware cũ | Không tương thích với ID `0x70` |
| Firmware mới | Nhận cảm biến và trigger thành công |
| Telegram | Nhận được cảnh báo |

Lỗi đã được xử lý bằng cách làm firmware tương thích với cả MPU6050 và MPU6500.
Không cần thay ESP32, thay dây hoặc thay cảm biến hiện tại.
