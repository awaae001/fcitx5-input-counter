// SPDX-License-Identifier: MIT

//! Implements custom chart range validation and calendar-aware bucketing.

#include "chart_range.h"

#include <algorithm>
#include <utility>

#include <QTime>

namespace inputcounter {

namespace {

QDateTime floorToHour(QDateTime value) {
  value.setTime(QTime(value.time().hour(), 0));
  return value;
}

QDateTime ceilToHour(const QDateTime &value) {
  auto result = floorToHour(value);
  if (result < value) {
    result = result.addSecs(60 * 60);
  }
  return result;
}

QDateTime advance(const QDateTime &value, ChartScale scale) {
  switch (scale) {
  case ChartScale::OneHour:
    return value.addSecs(60 * 60);
  case ChartScale::SixHours:
    return value.addSecs(6 * 60 * 60);
  case ChartScale::TwelveHours:
    return value.addSecs(12 * 60 * 60);
  case ChartScale::OneDay:
    return value.addDays(1);
  case ChartScale::OneWeek:
    return value.addDays(7);
  case ChartScale::OneMonth:
    return value.addMonths(1);
  }
  return {};
}

} // namespace

ChartRange::ChartRange(ChartScale scale, std::vector<TimeBucket> buckets)
    : scale_(scale), buckets_(std::move(buckets)) {}

std::variant<ChartRange, ChartRangeError>
ChartRange::create(QDateTime start, QDateTime end, ChartScale scale) {
  if (!start.isValid() || !end.isValid()) {
    return ChartRangeError::InvalidTime;
  }

  start = floorToHour(std::move(start));
  end = ceilToHour(std::move(end));
  if (end <= start) {
    return ChartRangeError::EndNotAfterStart;
  }

  std::vector<TimeBucket> buckets;
  buckets.reserve(std::min<std::size_t>(64, kMaximumBuckets));
  auto cursor = start;
  while (cursor < end) {
    if (buckets.size() == kMaximumBuckets) {
      return ChartRangeError::TooManyBuckets;
    }

    const auto next = advance(cursor, scale);
    if (!next.isValid() || next <= cursor) {
      return ChartRangeError::InvalidTime;
    }
    buckets.push_back({cursor, std::min(next, end)});
    cursor = next;
  }
  return ChartRange(scale, std::move(buckets));
}

} // namespace inputcounter
