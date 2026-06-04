#include "motion.h"

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "config.h"
#include "core/motion_filter.h"

namespace lapguard {
namespace {
constexpr uint32_t kSampleIntervalMs = 20UL;

Adafruit_MPU6050 mpu;
bool mpu_ready = false;
bool trigger_latched = false;
unsigned long last_sample_ms = 0;
MotionFilter motion_filter(MOTION_THRESHOLD_G, MOTION_PERSISTENCE_N);
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
  motion_filter.reset();
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