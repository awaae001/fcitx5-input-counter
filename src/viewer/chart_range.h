// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_CHART_RANGE_H
#define FCITX5_INPUT_COUNTER_VIEWER_CHART_RANGE_H

//! Models validated custom chart ranges and their bucket boundaries.

#include <cstddef>
#include <variant>
#include <vector>

#include <QDateTime>

namespace inputcounter {

/// Granularity used to aggregate a custom chart.
enum class ChartScale {
  OneHour,
  SixHours,
  TwelveHours,
  OneDay,
  OneWeek,
  OneMonth,
};

/// One half-open time bucket: [start, end).
struct TimeBucket final {
  /// Inclusive bucket start in local time.
  QDateTime start;
  /// Exclusive bucket end in local time.
  QDateTime end;
};

/// Reason a custom chart range could not be constructed.
enum class ChartRangeError {
  InvalidTime,
  EndNotAfterStart,
  TooManyBuckets,
};

/// A validated, non-empty custom range split into a bounded number of buckets.
class ChartRange final {
public:
  /// Maximum number of bars accepted by a custom chart.
  static constexpr std::size_t kMaximumBuckets = 512;

  /// Constructs a range after rounding start down and end up to whole hours.
  ///
  /// Returns an error when either timestamp is invalid, end is not after start,
  /// or the chosen scale would produce more than kMaximumBuckets bars.
  static std::variant<ChartRange, ChartRangeError>
  create(QDateTime start, QDateTime end, ChartScale scale);

  /// Returns the normalized inclusive range start.
  const QDateTime &start() const { return buckets_.front().start; }
  /// Returns the normalized exclusive range end.
  const QDateTime &end() const { return buckets_.back().end; }
  /// Returns the aggregation scale.
  ChartScale scale() const { return scale_; }
  /// Returns ordered, contiguous half-open buckets covering the range.
  const std::vector<TimeBucket> &buckets() const { return buckets_; }

private:
  ChartRange(ChartScale scale, std::vector<TimeBucket> buckets);

  ChartScale scale_;
  std::vector<TimeBucket> buckets_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_CHART_RANGE_H
