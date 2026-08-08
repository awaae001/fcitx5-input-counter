// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_STATISTICS_SNAPSHOT_H
#define FCITX5_INPUT_COUNTER_VIEWER_STATISTICS_SNAPSHOT_H

//! Converts stored counts into data ready for the viewer.

#include <cstdint>
#include <utility>
#include <vector>

#include <QString>

namespace inputcounter {

class ChartRange;
class StatsDb;

/// Labeled values displayed by a bar chart.
using ChartBars = std::vector<std::pair<QString, std::uint64_t>>;

/// Labeled values displayed by one main-window refresh.
struct StatisticsSnapshot final {
  /// All recorded characters.
  std::uint64_t total;
  /// Characters recorded today.
  std::uint64_t today;
  /// Characters recorded in the rolling 24-hour range.
  std::uint64_t last24Hours;
  /// Characters recorded in the seven-day range.
  std::uint64_t last7Days;
  /// Hourly bars.
  std::vector<std::pair<QString, std::uint64_t>> hours;
  /// Seven-day bars aggregated at six-hour intervals.
  ChartBars week;
  /// Thirty-day bars.
  std::vector<std::pair<QString, std::uint64_t>> month;
  /// Twelve-month bars.
  std::vector<std::pair<QString, std::uint64_t>> lastYear;
  /// Yearly bars covering all recorded history.
  std::vector<std::pair<QString, std::uint64_t>> allTime;
};

/// Returns the current Unix timestamp in seconds.
std::int64_t nowSeconds();

/// Reads and aggregates one display snapshot at now.
StatisticsSnapshot load(StatsDb &db, std::int64_t now);

/// Reads and aggregates bars for a validated custom range.
ChartBars load(StatsDb &db, const ChartRange &range);

/// Formats a count with the current locale.
QString format(std::uint64_t value);

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_STATISTICS_SNAPSHOT_H
