// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_STATISTICS_TYPES_H
#define FCITX5_INPUT_COUNTER_STATISTICS_TYPES_H

//! Defines the domain values used by statistics persistence and queries.

#include <cstddef>
#include <cstdint>

namespace inputcounter {

/// Maximum number of buckets accepted by one statistics request.
inline constexpr std::size_t kMaximumStatisticsBuckets = 512;

/// Rounds a Unix timestamp down to the start of its hour.
constexpr std::int64_t hourStartOf(std::int64_t unixSeconds) {
  const auto remainder = unixSeconds % 3600;
  return unixSeconds - remainder - (remainder < 0 ? 3600 : 0);
}

/// One hourly character count.
struct HourlyCount final {
  std::int64_t hour;
  std::uint64_t chars;
};

struct TimeRange final {
  std::int64_t start;
  std::int64_t end;
};

struct StatisticsSummary final {
  std::uint64_t total;
  std::uint64_t today;
  std::uint64_t last24Hours;
  std::uint64_t last7Days;
  bool hasData;
  std::int64_t firstHour;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_STATISTICS_TYPES_H
