#pragma once

#include <cctype>
#include <cstddef>
#include <string>

namespace lapguard {
inline bool is_valid_pin_format(const std::string& pin, std::size_t min_len, std::size_t max_len) {
	if (pin.length() < min_len || pin.length() > max_len) {
		return false;
	}

	for (unsigned char character : pin) {
		if (!std::isdigit(character)) {
			return false;
		}
	}

	return true;
}
}  // namespace lapguard