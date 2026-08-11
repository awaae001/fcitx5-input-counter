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

} // namespace

ChartRange::ChartRange(ChartScale scale, std::vector<TimeBucket> buckets)
    : scale_(scale), buckets_(std::move(buckets)) {}

std::variant<ChartRange, ChartRangeError>
ChartRange::create(QDateTime start, QDateTime end, ChartScale scale) {
  if (!start.isValid() || !end.isValid()) {
    return ChartRangeError::InvalidTime;
  }

  start = floorToHour(std::move(start));
  const auto requestedEnd = end;
  end = floorToHour(std::move(end));
  if (end < requestedEnd) {
    end = end.addSecs(60 * 60);
  }
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

    QDateTime next;
    switch (scale) {
    case ChartScale::OneHour:
      next = cursor.addSecs(60 * 60);
      break;
    case ChartScale::SixHours:
      next = cursor.addSecs(6 * 60 * 60);
      break;
    case ChartScale::TwelveHours:
      next = cursor.addSecs(12 * 60 * 60);
      break;
    case ChartScale::OneDay:
      next = cursor.addDays(1);
      break;
    case ChartScale::OneWeek:
      next = cursor.addDays(7);
      break;
    case ChartScale::OneMonth:
      next = cursor.addMonths(1);
      break;
    }
    if (!next.isValid() || next <= cursor) {
      return ChartRangeError::InvalidTime;
    }
    buckets.push_back({cursor, std::min(next, end)});
    cursor = next;
  }
  return ChartRange(scale, std::move(buckets));
}

} // namespace inputcounter
