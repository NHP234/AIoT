# Sơ đồ nối chân thiết bị chống trộm LapGuard (ESP32)

Tài liệu này cung cấp sơ đồ nối dây chi tiết và nhanh chóng phục vụ cho việc lắp ráp mạch phần cứng của thiết bị LapGuard.

---

## 1. Bảng nối chân tổng hợp

| Linh kiện | Chân linh kiện | Chân kết nối ESP32 | Loại tín hiệu | Điện áp | Ghi chú |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **MPU6050** | VCC | **3V3** | Nguồn cấp | 3.3V | Cấp nguồn từ chân 3V3 của ESP32 |
| | GND | **GND** | Nguồn cấp | 0V | GND chung toàn mạch |
| | SCL | **GPIO 22** | I2C SCL | 3.3V | Chân CLK của bus I2C |
| | SDA | **GPIO 21** | I2C SDA | 3.3V | Chân DATA của bus I2C |
| | INT | **GPIO 15** | Input Interrupt | 3.3V | Báo ngắt khi có chuyển động |
| | AD0 | **GND** | Địa chỉ I2C | 0V | Đặt địa chỉ I2C cố định là `0x68` |
| **Còi báo (Buzzer)** | VCC | **VIN** (hoặc 5V) | Nguồn cấp | 5.0V | Nguồn lấy sau mạch tăng áp MT3608 |
| *(Module 3 chân)* | GND | **GND** | Nguồn cấp | 0V | GND chung toàn mạch |
| | IN (IO) | **GPIO 25** | Digital Output | 3.3V | Xuất tín hiệu kích kêu còi (Active High) |
| **LED Xanh (OK)** | Anode (+) | **GPIO 26** | Digital Output | 3.3V | Nối qua **Điện trở 220 Ohm** |
| | Cathode (-) | **GND** | Nguồn cấp | 0V | GND chung toàn mạch |
| **LED Đỏ (Alert)** | Anode (+) | **GPIO 27** | Digital Output | 3.3V | Nối qua **Điện trở 220 Ohm** |
| | Cathode (-) | **GND** | Nguồn cấp | 0V | GND chung toàn mạch |
| **Cầu phân áp đo Pin** | Điểm giữa | **GPIO 34** | Analog Input | Max 2.2V | Cầu phân áp hai điện trở **100k / 100k** |

---

## 2. Sơ đồ khối và Luồng cấp nguồn

```
[Pin Lithium 18650 (3.7V - 4.2V)] 
             │
             ▼
[Module sạc TP4056 (Cổng sạc Type-C)]
             │
      (OUT+) ├────────► [Công tắc gạt chính] ──► [Mạch tăng áp MT3608 (IN+)]
      (OUT-) └──────────────────────────────────► [Mạch tăng áp MT3608 (IN-)]
                                                                │
                                                         (OUT+ 5V / OUT-)
                                                                │
                                                                ▼
                                                       [Chân VIN / GND ESP32]
```

---

## 3. Hướng dẫn nối dây chi tiết từng khối

### Khối Cảm biến (MPU6050)
1. Nối chân **VCC** của MPU6050 vào chân **3V3** của ESP32.
2. Nối chân **GND** của MPU6050 vào chân **GND** của ESP32.
3. Nối chân **SCL** của MPU6050 vào chân **GPIO 22** của ESP32.
4. Nối chân **SDA** của MPU6050 vào chân **GPIO 21** của ESP32.
5. Nối chân **INT** của MPU6050 vào chân **GPIO 15** của ESP32.
6. Nối chân **AD0** của MPU6050 vào **GND** (để khóa địa chỉ I2C ở `0x68`).

### Khối Báo động (Còi & LED)
1. **Module Còi 3 chân**:
   * Nối chân **VCC** còi vào chân **VIN** của ESP32 (để lấy nguồn 5V mạnh cho còi hú to).
   * Nối chân **GND** còi vào chân **GND** của ESP32.
   * Nối chân **I/O** (hoặc **IN**) còi vào chân **GPIO 25** của ESP32.
2. **LED Xanh (Trạng thái AN TOÀN / BẢO VỆ)**:
   * Chân dài (Anode +) -> nối vào một đầu **Điện trở 220 Ohm** -> đầu còn lại điện trở nối vào **GPIO 26** của ESP32.
   * Chân ngắn (Cathode -) -> nối vào **GND** của ESP32.
3. **LED Đỏ (Trạng thái CẢNH BÁO)**:
   * Chân dài (Anode +) -> nối vào một đầu **Điện trở 220 Ohm** -> đầu còn lại điện trở nối vào **GPIO 27** của ESP32.
   * Chân ngắn (Cathode -) -> nối vào **GND** của ESP32.

### Khối Đo Điện Áp Pin (Cầu phân áp)
Để đọc dung lượng pin 18650 (3V - 4.2V) mà không làm cháy cổng analog ESP32 (chỉ chịu được tối đa 3.3V):
1. Nối một đầu điện trở thứ nhất **R1 (100k Ohm)** vào cực dương pin 18650 (trước công tắc).
2. Nối đầu còn lại của **R1** vào chân **GPIO 34** của ESP32.
3. Nối một đầu điện trở thứ hai **R2 (100k Ohm)** vào chân **GPIO 34** của ESP32.
4. Nối đầu còn lại của **R2** vào chân **GND** của ESP32.
5. *(Khuyến nghị)*: Nối song song một tụ điện nhỏ **100nF** giữa chân **GPIO 34** và **GND** để lọc nhiễu đọc analog.

---

## 4. Lưu ý sống còn khi ráp mạch
1. > [!CAUTION]
   > **Cấu hình MT3608 trước khi cắm ESP32**: Trước khi nối đầu ra `OUT+` của module tăng áp MT3608 vào chân `VIN` của ESP32, bác bắt buộc phải bật nguồn pin, dùng đồng hồ đo điện áp đo ở hai đầu `OUT+` và `OUT-` của MT3608. Vặn biến trở trên mạch MT3608 ngược chiều kim đồng hồ cho đến khi đồng hồ đo hiển thị đúng **5.0V** rồi mới được tắt nguồn và tiến hành hàn/cắm dây vào ESP32. Nếu điện áp ra vượt quá 6V sẽ gây cháy chip ESP32 lập tức.
2. **Không dùng chân Boot**: Tránh đấu nối còi hay LED vào các chân GPIO 0, 2, 12, 15 khi khởi động để tránh làm chip không vào được chế độ nạp chương trình (Bootloader).
3. **GND chung**: Tất cả các linh kiện (ESP32, MPU6050, Còi, LED, Cầu phân áp, Module sạc/tăng áp) bắt buộc phải nối chung đường cực âm (**GND**) để tín hiệu không bị nhiễu loạn.
