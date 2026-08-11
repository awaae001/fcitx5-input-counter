// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_STATISTICS_TYPES_H
#define FCITX5_INPUT_COUNTER_STATISTICS_TYPES_H

//! Defines statistics query values shared across process boundaries.

#include <cstdint>

namespace inputcounter {

/// One half-open Unix-time range: [start, end).
struct TimeRange final {
  /// Inclusive Unix timestamp.
  std::int64_t start;
  /// Exclusive Unix timestamp.
  std::int64_t end;
};

/// Aggregate values displayed in the viewer overview.
struct StatisticsSummary final {
  /// All recorded characters.
  std::uint64_t total;
  /// Characters recorded since the start of the local day.
  std::uint64_t today;
  /// Characters recorded in the requested rolling 24-hour range.
  std::uint64_t last24Hours;
  /// Characters recorded in the requested seven-day range.
  std::uint64_t last7Days;
  /// Whether at least one persisted hourly bucket exists.
  bool hasData;
  /// Earliest persisted hour; meaningful only when hasData is true.
  std::int64_t firstHour;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_STATISTICS_TYPES_H
