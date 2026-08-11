// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_CHART_RANGE_H
#define FCITX5_INPUT_COUNTER_VIEWER_CHART_RANGE_H

//! Models validated custom chart ranges and their bucket boundaries.

#include <cstddef>
#include <variant>
#include <vector>

#include <QDateTime>

#include "../statistics_types.h"

namespace inputcounter
{

  /// Granularity used to aggregate a custom chart.
  enum class ChartScale
  {
    OneHour,
    SixHours,
    TwelveHours,
    OneDay,
    OneWeek,
    OneMonth,
  };

  struct TimeBucket final
  {
    QDateTime start;
    QDateTime end;
  };

  /// Reason a custom chart range could not be constructed.
  enum class ChartRangeError
  {
    InvalidTime,
    EndNotAfterStart,
    TooManyBuckets,
  };

  /// A validated, non-empty custom range split into a bounded number of buckets.
  class ChartRange final
  {
  public:
    static constexpr std::size_t kMaximumBuckets = kMaximumStatisticsBuckets;

    /// Constructs a range after rounding start down and end up to whole hours.
    ///
    /// Returns an error when either timestamp is invalid, end is not after start,
    /// or the chosen scale would produce more than kMaximumBuckets bars.
    static std::variant<ChartRange, ChartRangeError>
    create(QDateTime start, QDateTime end, ChartScale scale);

    const QDateTime &start() const { return buckets_.front().start; }
    const QDateTime &end() const { return buckets_.back().end; }
    ChartScale scale() const { return scale_; }

    const std::vector<TimeBucket> &buckets() const { return buckets_; }

  private:
    ChartRange(ChartScale scale, std::vector<TimeBucket> buckets);

    ChartScale scale_;
    std::vector<TimeBucket> buckets_;
  };

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_CHART_RANGE_H
