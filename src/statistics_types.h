// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_STATISTICS_TYPES_H
#define FCITX5_INPUT_COUNTER_STATISTICS_TYPES_H

//! Defines statistics query values shared across process boundaries.

#include <cstddef>
#include <cstdint>

namespace inputcounter {

/// Maximum number of buckets accepted by one statistics request.
inline constexpr std::size_t kMaximumStatisticsBuckets = 512;

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
