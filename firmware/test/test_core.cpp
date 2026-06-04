#include <unity.h>

#include <string>

#include "core/motion_filter.h"
#include "core/pin_policy.h"

using namespace lapguard;

void setUp(void) {}

void tearDown(void) {}

void test_pin_policy_valid_numbers() {
  TEST_ASSERT_TRUE(is_valid_pin_format("1234", 4, 8));
  TEST_ASSERT_TRUE(is_valid_pin_format("12345678", 4, 8));
}

void test_pin_policy_rejects_invalid_values() {
  TEST_ASSERT_FALSE(is_valid_pin_format("123", 4, 8));
  TEST_ASSERT_FALSE(is_valid_pin_format("123456789", 4, 8));
  TEST_ASSERT_FALSE(is_valid_pin_format("12ab", 4, 8));
}

void test_motion_filter_triggers_after_persistence() {
  MotionFilter filter(0.30f, 3);
  filter.reset();

  TEST_ASSERT_FALSE(filter.push_sample(0.50f));
  TEST_ASSERT_FALSE(filter.push_sample(0.50f));
  TEST_ASSERT_TRUE(filter.push_sample(0.50f));
  TEST_ASSERT_TRUE(filter.triggered());
  TEST_ASSERT_EQUAL_UINT8(3, filter.consecutive_count());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.50f, filter.average());
}

void test_motion_filter_resets_on_low_input() {
  MotionFilter filter(0.30f, 3);
  filter.reset();

  filter.push_sample(0.50f);
  filter.push_sample(0.50f);
  TEST_ASSERT_FALSE(filter.triggered());
  TEST_ASSERT_TRUE(filter.push_sample(0.50f));
  TEST_ASSERT_TRUE(filter.triggered());
  filter.push_sample(0.00f);
  filter.push_sample(0.00f);
  filter.push_sample(0.00f);
  TEST_ASSERT_FALSE(filter.triggered());
  TEST_ASSERT_EQUAL_UINT8(0, filter.consecutive_count());
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(test_pin_policy_valid_numbers);
  RUN_TEST(test_pin_policy_rejects_invalid_values);
  RUN_TEST(test_motion_filter_triggers_after_persistence);
  RUN_TEST(test_motion_filter_resets_on_low_input);
  return UNITY_END();
}