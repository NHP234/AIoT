#include "motion.h"

#include <Wire.h>

#include "config.h"
#include "core/motion_filter.h"

namespace lapguard {
namespace {
constexpr uint32_t kSampleIntervalMs = 20UL;
constexpr uint8_t kPrimaryAddress = 0x68;
constexpr uint8_t kSecondaryAddress = 0x69;
constexpr uint8_t kMpu6050DeviceId = 0x68;
constexpr uint8_t kMpu6500DeviceId = 0x70;

constexpr uint8_t kSampleRateDividerRegister = 0x19;
constexpr uint8_t kConfigRegister = 0x1A;
constexpr uint8_t kGyroConfigRegister = 0x1B;
constexpr uint8_t kAccelConfigRegister = 0x1C;
constexpr uint8_t kAccelConfig2Register = 0x1D;
constexpr uint8_t kAccelDataRegister = 0x3B;
constexpr uint8_t kPowerManagement1Register = 0x6B;
constexpr uint8_t kPowerManagement2Register = 0x6C;
constexpr uint8_t kWhoAmIRegister = 0x75;
constexpr float kAccelScaleLsbPerG = 8192.0f;
constexpr float kGyroScaleLsbPerDps = 65.5f;
#ifdef LAPGUARD_SENSOR_TEST
constexpr uint32_t kDiagnosticPrintIntervalMs = 100UL;
#endif

bool mpu_ready = false;
bool trigger_latched = false;
uint8_t mpu_address = kPrimaryAddress;
uint8_t mpu_device_id = 0;
unsigned long last_sample_ms = 0;
#ifdef LAPGUARD_SENSOR_TEST
unsigned long last_diagnostic_print_ms = 0;
uint32_t diagnostic_read_errors = 0;
#endif
MotionFilter motion_filter(MOTION_THRESHOLD_G, MOTION_PERSISTENCE_N);

bool read_registers(uint8_t address, uint8_t reg, uint8_t* data, size_t length) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  if (Wire.requestFrom(address, static_cast<uint8_t>(length)) != length) {
    return false;
  }

  for (size_t index = 0; index < length; ++index) {
    data[index] = Wire.read();
  }
  return true;
}

bool write_register(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(mpu_address);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool detect_sensor_at(uint8_t address) {
  uint8_t device_id = 0;
  if (!read_registers(address, kWhoAmIRegister, &device_id, 1)) {
    return false;
  }

  if (device_id != kMpu6050DeviceId && device_id != kMpu6500DeviceId) {
    Serial.printf("[MOTION] Unsupported sensor at 0x%02X, WHO_AM_I=0x%02X\n",
                  address, device_id);
    return false;
  }

  mpu_address = address;
  mpu_device_id = device_id;
  return true;
}

bool detect_sensor() {
  for (uint8_t attempt = 0; attempt < 3; ++attempt) {
    if (detect_sensor_at(kPrimaryAddress) || detect_sensor_at(kSecondaryAddress)) {
      return true;
    }
    delay(50);
  }
  return false;
}

bool configure_sensor() {
  // Some MPU-6500 modules stop acknowledging as soon as reset starts.
  write_register(kPowerManagement1Register, 0x80);
  delay(300);

  if (!write_register(kPowerManagement1Register, 0x01) ||
      !write_register(kPowerManagement2Register, 0x00) ||
      !write_register(kSampleRateDividerRegister, 19) ||
      !write_register(kConfigRegister, 0x03) ||
      !write_register(kGyroConfigRegister, 0x08) ||
      !write_register(kAccelConfigRegister, 0x08)) {
    return false;
  }

  if (mpu_device_id == kMpu6500DeviceId &&
      !write_register(kAccelConfig2Register, 0x03)) {
    return false;
  }

  delay(100);
  return true;
}
}  // namespace

void motion_init() {
  Wire.begin(I2C_SDA, I2C_SCL, 100000);
  Wire.setTimeOut(50);

  mpu_ready = detect_sensor() && configure_sensor();
  if (!mpu_ready) {
    Serial.println(F("[MOTION] MPU6050/MPU6500 init failed"));
  } else {
    Serial.printf("[MOTION] %s initialized at 0x%02X (WHO_AM_I=0x%02X)\n",
                  mpu_device_id == kMpu6500DeviceId ? "MPU6500" : "MPU6050",
                  mpu_address, mpu_device_id);
  }

  motion_reset();
}

void motion_reset() {
  motion_filter.reset();
  trigger_latched = false;
  last_sample_ms = 0;
#ifdef LAPGUARD_SENSOR_TEST
  last_diagnostic_print_ms = 0;
  diagnostic_read_errors = 0;
#endif
}

bool motion_sensor_ready() {
  return mpu_ready;
}

bool motion_poll() {
  if (!mpu_ready) {
    return false;
  }

  const unsigned long now = millis();
  if (now - last_sample_ms < kSampleIntervalMs) {
    return false;
  }
  last_sample_ms = now;

  uint8_t raw_data[14] = {};
  if (!read_registers(mpu_address, kAccelDataRegister, raw_data, sizeof(raw_data))) {
#ifdef LAPGUARD_SENSOR_TEST
    ++diagnostic_read_errors;
#endif
    Serial.println(F("[MOTION] Accelerometer read failed"));
    return false;
  }

  const int16_t raw_x = static_cast<int16_t>((raw_data[0] << 8) | raw_data[1]);
  const int16_t raw_y = static_cast<int16_t>((raw_data[2] << 8) | raw_data[3]);
  const int16_t raw_z = static_cast<int16_t>((raw_data[4] << 8) | raw_data[5]);
  const float accel_x_g = raw_x / kAccelScaleLsbPerG;
  const float accel_y_g = raw_y / kAccelScaleLsbPerG;
  const float accel_z_g = raw_z / kAccelScaleLsbPerG;
  const float magnitude_g = sqrtf(
    accel_x_g * accel_x_g +
    accel_y_g * accel_y_g +
    accel_z_g * accel_z_g);
  const float delta_g = fabsf(magnitude_g - 1.0f);

  const bool triggered_now = motion_push_sample(delta_g);

#ifdef LAPGUARD_SENSOR_TEST
  if (now - last_diagnostic_print_ms >= kDiagnosticPrintIntervalMs) {
    const int16_t raw_gyro_x = static_cast<int16_t>((raw_data[8] << 8) | raw_data[9]);
    const int16_t raw_gyro_y = static_cast<int16_t>((raw_data[10] << 8) | raw_data[11]);
    const int16_t raw_gyro_z = static_cast<int16_t>((raw_data[12] << 8) | raw_data[13]);
    Serial.printf(
      "ax:%.3f\tay:%.3f\taz:%.3f\tmagnitude:%.3f\tdelta:%.3f\t"
      "average:%.3f\tthreshold:%.3f\tgx:%.1f\tgy:%.1f\tgz:%.1f\terrors:%lu\n",
      accel_x_g, accel_y_g, accel_z_g, magnitude_g, delta_g,
      motion_average(), MOTION_THRESHOLD_G,
      raw_gyro_x / kGyroScaleLsbPerDps,
      raw_gyro_y / kGyroScaleLsbPerDps,
      raw_gyro_z / kGyroScaleLsbPerDps,
      diagnostic_read_errors);
    last_diagnostic_print_ms = now;
  }
#endif

  if (triggered_now && !trigger_latched) {
    trigger_latched = true;
    return true;
  }

  if (!motion_triggered()) {
    trigger_latched = false;
  }

  return false;
}

bool motion_push_sample(float delta_g) {
  return motion_filter.push_sample(delta_g);
}

bool motion_triggered() {
  return motion_filter.triggered();
}

float motion_average() {
  return motion_filter.average();
}

uint8_t motion_consecutive_count() {
  return motion_filter.consecutive_count();
}
}  // namespace lapguard
