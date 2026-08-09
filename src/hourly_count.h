// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_HOURLY_COUNT_H
#define FCITX5_INPUT_COUNTER_HOURLY_COUNT_H

//! Defines the hourly value type shared by persistence and presentation.

#include <cstdint>

namespace inputcounter {

/// Rounds a Unix timestamp down to the start of its hour.
constexpr std::int64_t hourStartOf(std::int64_t unixSeconds) {
  return unixSeconds - unixSeconds % 3600;
}

/// One persisted hourly character count.
struct HourlyCount final {
  /// Unix timestamp at the start of the represented hour.
  std::int64_t hour;
  /// Number of characters recorded during the represented hour.
  std::uint64_t chars;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_HOURLY_COUNT_H
