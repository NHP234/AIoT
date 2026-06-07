# Kiểm tra MPU6050/MPU6500

## Mục tiêu

Chế độ `esp32dev-sensor-test` kiểm tra trực tiếp cảm biến, không chạy WiFi,
Telegram, FSM, còi hoặc LED. Firmware lấy mẫu ở 50 Hz và xuất dữ liệu 10 Hz qua
Serial để đánh giá kết nối, từng trục đo, nhiễu và độ nhạy.

## Cách chạy trong VSCode

1. Cắm ESP32 và đóng mọi Serial Monitor đang mở.
2. Mở PlatformIO ở thanh bên.
3. Mở `PROJECT TASKS > esp32dev-sensor-test`.
4. Chọn `Upload`.
5. Sau khi nạp xong, chọn `Monitor`.

Có thể dùng `PlatformIO: Serial Plotter` để xem biểu đồ. Baud rate là `115200`.

## Dữ liệu hiển thị

```text
ax:... ay:... az:... magnitude:... delta:... average:... threshold:...
gx:... gy:... gz:... errors:...
```

- `ax`, `ay`, `az`: gia tốc từng trục, đơn vị `g`.
- `magnitude`: độ lớn vector gia tốc.
- `delta`: độ lệch tức thời so với `1g`.
- `average`: giá trị sau bộ lọc dùng để trigger.
- `threshold`: ngưỡng trigger hiện tại, mặc định `0.300g`.
- `gx`, `gy`, `gz`: tốc độ quay từng trục, đơn vị độ/giây.
- `errors`: tổng số lần đọc I2C thất bại.

## Quy trình đánh giá

### 1. Để cảm biến đứng yên trong 10 giây

Kết quả bình thường:

- `magnitude` gần `1.0g`.
- `delta` và `average` thấp hơn nhiều so với `0.300g`.
- `errors` giữ nguyên ở `0`.
- Gyro có thể lệch vài độ/giây do chưa hiệu chuẩn.

Nếu `average` thường xuyên gần hoặc vượt `0.300g` khi đứng yên, cần kiểm tra
nguồn, dây nối, mối hàn hoặc giảm nhiễu cơ khí trước khi chỉnh ngưỡng.

### 2. Xoay lần lượt từng mặt

Khi một trục hướng thẳng theo trọng lực, trục đó phải tiến gần `+1g` hoặc `-1g`;
hai trục còn lại tiến gần `0g`. Thực hiện với cả ba trục để phát hiện trục bị
kẹt hoặc trả dữ liệu bất thường.

### 3. Xoay cảm biến bằng tay

`gx`, `gy`, `gz` phải thay đổi theo trục đang xoay và trở về gần giá trị ban đầu
khi dừng. Bước này xác nhận phần gyro của chip hoạt động.

### 4. Giật hoặc lắc ngắn

`delta` phải tăng ngay. `average` vượt đường `threshold` trong ít nhất ba mẫu sẽ
tương ứng với một motion trigger trong firmware chính.

## Tiêu chí cảm biến đạt

- Khởi tạo đúng MPU6050 hoặc MPU6500.
- `errors = 0` trong toàn bộ bài test.
- Cả ba trục gia tốc phản ứng khi đổi hướng.
- Cả ba trục gyro phản ứng khi xoay.
- Khi đứng yên, độ lớn gia tốc gần `1g` và không tự vượt ngưỡng.
- Khi giật/lắc, `average` vượt `0.300g` rõ ràng.

Sau khi kiểm tra xong, chọn `PROJECT TASKS > esp32dev > Upload` để nạp lại
firmware LapGuard chính thức.
