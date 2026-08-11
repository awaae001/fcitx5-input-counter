// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_HOURLY_COUNT_H
#define FCITX5_INPUT_COUNTER_HOURLY_COUNT_H

//! Defines the hourly value type shared by persistence and presentation.

#include <cstdint>

namespace inputcounter
{

  /// Rounds a Unix timestamp down to the start of its hour.
  constexpr std::int64_t hourStartOf(std::int64_t unixSeconds)
  {
    const auto remainder = unixSeconds % 3600;
    return unixSeconds - remainder - (remainder < 0 ? 3600 : 0);
  }

  struct HourlyCount final
  {
    std::int64_t hour;
    std::uint64_t chars;
  };

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_HOURLY_COUNT_H
