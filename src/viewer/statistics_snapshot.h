// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_STATISTICS_SNAPSHOT_H
#define FCITX5_INPUT_COUNTER_VIEWER_STATISTICS_SNAPSHOT_H

//! Builds bounded statistics queries and display-ready chart values.

#include <cstdint>
#include <utility>
#include <vector>

#include <QString>

#include "../statistics_types.h"

namespace inputcounter
{

  class ChartRange;

  struct SummaryQuery final
  {
    std::int64_t todayStart;
    std::int64_t last24HoursStart;
    std::int64_t last7DaysStart;
  };

  struct ChartBucket final
  {
    TimeRange range;
    QString label;
  };

  /// Ordered chart buckets sent to the addon.
  using ChartQuery = std::vector<ChartBucket>;

  /// Labeled values displayed by a bar chart.
  using ChartBars = std::vector<std::pair<QString, std::uint64_t>>;

  SummaryQuery summaryQuery(std::int64_t now);
  ChartQuery last24HoursQuery(std::int64_t now);
  ChartQuery last7DaysQuery(std::int64_t now);
  ChartQuery last30DaysQuery(std::int64_t now);
  ChartQuery last12MonthsQuery(std::int64_t now);
  ChartQuery allTimeQuery(std::int64_t firstHour, std::int64_t now);
  ChartQuery customQuery(const ChartRange &range);

  /// Extracts the wire ranges from a chart query.
  std::vector<TimeRange> ranges(const ChartQuery &query);

  /// Pairs returned bucket counts with their query labels.
  ///
  /// Throws std::invalid_argument when the result cardinality does not match the
  /// query cardinality.
  ChartBars makeBars(const ChartQuery &query,
                     const std::vector<std::uint64_t> &counts);

  /// Formats a count with the current locale.
  QString format(std::uint64_t value);

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_STATISTICS_SNAPSHOT_H
