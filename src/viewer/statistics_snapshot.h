// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_STATISTICS_SNAPSHOT_H
#define FCITX5_INPUT_COUNTER_VIEWER_STATISTICS_SNAPSHOT_H

//! Builds bounded statistics queries and display-ready chart values.

#include <cstdint>
#include <utility>
#include <vector>

#include <QString>

#include "../statistics_types.h"

namespace inputcounter {

class ChartRange;

/// Time boundaries used to calculate the overview values.
struct SummaryQuery final {
  /// Start of the current local day.
  std::int64_t todayStart;
  /// Start used for the rolling 24-hour total.
  std::int64_t last24HoursStart;
  /// Start used for the seven-day total.
  std::int64_t last7DaysStart;
};

/// One chart query bucket and its presentation label.
struct ChartBucket final {
  /// Time range summed by the addon.
  TimeRange range;
  /// Label displayed below the resulting bar.
  QString label;
};

/// Ordered chart buckets sent to the addon.
using ChartQuery = std::vector<ChartBucket>;

/// Labeled values displayed by a bar chart.
using ChartBars = std::vector<std::pair<QString, std::uint64_t>>;

/// Returns the current Unix timestamp in seconds.
std::int64_t nowSeconds();

/// Builds overview time boundaries at now.
SummaryQuery summaryQuery(std::int64_t now);

/// Builds the 24 hourly buckets ending after the current hour.
ChartQuery last24HoursQuery(std::int64_t now);

/// Builds 28 six-hour buckets ending after the current hour.
ChartQuery last7DaysQuery(std::int64_t now);

/// Builds 30 local-calendar day buckets ending after today.
ChartQuery last30DaysQuery(std::int64_t now);

/// Builds 12 local-calendar month buckets ending after this month.
ChartQuery last12MonthsQuery(std::int64_t now);

/// Builds local-calendar year buckets from firstHour through this year.
ChartQuery allTimeQuery(std::int64_t firstHour, std::int64_t now);

/// Builds labeled buckets for a validated custom range.
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
