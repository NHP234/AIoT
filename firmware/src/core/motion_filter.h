#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace lapguard {
class MotionFilter {
 public:
  static constexpr std::size_t kBufferSize = 10;

  inline MotionFilter(float threshold_g, std::uint8_t persistence_n)
      : threshold_g_(threshold_g), persistence_n_(persistence_n) {}

  inline void reset() {
    sample_buffer_.fill(0.0f);
    sample_index_ = 0;
    sample_count_ = 0;
    consecutive_count_ = 0;
    average_delta_ = 0.0f;
  }

  inline bool push_sample(float delta_g) {
    sample_buffer_[sample_index_] = delta_g;
    sample_index_ = (sample_index_ + 1) % kBufferSize;
    if (sample_count_ < kBufferSize) {
      ++sample_count_;
    }

    average_delta_ = recompute_average();

    if (average_delta_ > threshold_g_) {
      if (consecutive_count_ < UINT8_MAX) {
        ++consecutive_count_;
      }
    } else {
      consecutive_count_ = 0;
    }

    return triggered();
  }

  inline bool triggered() const {
    return consecutive_count_ >= persistence_n_;
  }

  inline float average() const {
    return average_delta_;
  }

  inline std::uint8_t consecutive_count() const {
    return consecutive_count_;
  }

 private:
  inline float recompute_average() const {
    if (sample_count_ == 0) {
      return 0.0f;
    }

    float sum = 0.0f;
    for (std::size_t index = 0; index < sample_count_; ++index) {
      sum += sample_buffer_[index];
    }
    return sum / static_cast<float>(sample_count_);
  }

  float threshold_g_;
  std::uint8_t persistence_n_;
  std::array<float, kBufferSize> sample_buffer_{};
  std::size_t sample_index_ = 0;
  std::size_t sample_count_ = 0;
  std::uint8_t consecutive_count_ = 0;
  float average_delta_ = 0.0f;
};
}  // namespace lapguard