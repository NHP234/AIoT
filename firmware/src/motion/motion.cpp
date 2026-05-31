#include "motion.h"

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "config.h"

namespace lapguard {
namespace {
constexpr uint32_t kSampleIntervalMs = 20UL;
constexpr size_t kBufferSize = 10;

Adafruit_MPU6050 mpu;

float sample_buffer[kBufferSize] = {};
size_t sample_index = 0;
size_t sample_count = 0;
uint8_t consecutive_count = 0;
float average_delta = 0.0f;
bool mpu_ready = false;
bool trigger_latched = false;
unsigned long last_sample_ms = 0;

float recompute_average() {
  if (sample_count == 0) {
    return 0.0f;
  }

  float sum = 0.0f;
  for (size_t index = 0; index < sample_count; ++index) {
    sum += sample_buffer[index];
  }
  return sum / static_cast<float>(sample_count);
}
}  // namespace

void motion_init() {
  Wire.begin(I2C_SDA, I2C_SCL);

  mpu_ready = mpu.begin();
  if (mpu_ready) {
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
    Serial.println(F("[MOTION] MPU6050 initialized"));
  } else {
    Serial.println(F("[MOTION] MPU6050 init failed"));
  }

  motion_reset();
}

void motion_reset() {
  for (float& value : sample_buffer) {
    value = 0.0f;
  }

  sample_index = 0;
  sample_count = 0;
  consecutive_count = 0;
  average_delta = 0.0f;
  trigger_latched = false;
  last_sample_ms = 0;
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

  sensors_event_t accel_event;
  sensors_event_t gyro_event;
  sensors_event_t temp_event;
  mpu.getEvent(&accel_event, &gyro_event, &temp_event);

  const float magnitude = sqrtf(
    accel_event.acceleration.x * accel_event.acceleration.x +
    accel_event.acceleration.y * accel_event.acceleration.y +
    accel_event.acceleration.z * accel_event.acceleration.z);
  const float delta_g = fabsf(magnitude - 9.80665f) / 9.80665f;

  const bool triggered_now = motion_push_sample(delta_g);
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
  sample_buffer[sample_index] = delta_g;
  sample_index = (sample_index + 1) % kBufferSize;
  if (sample_count < kBufferSize) {
    ++sample_count;
  }

  average_delta = recompute_average();

  if (average_delta > MOTION_THRESHOLD_G) {
    if (consecutive_count < UINT8_MAX) {
      ++consecutive_count;
    }
  } else {
    consecutive_count = 0;
  }

  return motion_triggered();
}

bool motion_triggered() {
  return consecutive_count >= MOTION_PERSISTENCE_N;
}

float motion_average() {
  return average_delta;
}

uint8_t motion_consecutive_count() {
  return consecutive_count;
}
}  // namespace lapguard