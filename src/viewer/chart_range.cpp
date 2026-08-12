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

/// Rounds a requested end up to the next whole-hour boundary.
QDateTime ceilToHour(QDateTime value) {
  auto floored = floorToHour(value);
  if (floored < value) {
    floored = floored.addSecs(60 * 60);
  }
  return floored;
}

/// Returns the next bucket boundary after `cursor`, or an invalid QDateTime
/// when the scale cannot advance the cursor.
QDateTime advance(QDateTime cursor, ChartScale scale) {
  switch (scale) {
  case ChartScale::OneHour:
    return cursor.addSecs(60 * 60);
  case ChartScale::SixHours:
    return cursor.addSecs(6 * 60 * 60);
  case ChartScale::TwelveHours:
    return cursor.addSecs(12 * 60 * 60);
  case ChartScale::OneDay:
    return cursor.addDays(1);
  case ChartScale::OneWeek:
    return cursor.addDays(7);
  case ChartScale::OneMonth:
    return cursor.addMonths(1);
  }
  return {};
}

} // namespace

ChartRange::ChartRange(ChartScale scale, std::vector<TimeBucket> buckets)
    : scale_(scale), buckets_(std::move(buckets)) {}

std::size_t ChartRange::countBuckets(QDateTime start, QDateTime end,
                                     ChartScale scale) {
  if (!start.isValid() || !end.isValid()) {
    return 0;
  }

  start = floorToHour(std::move(start));
  end = ceilToHour(std::move(end));
  if (end <= start) {
    return 0;
  }

  std::size_t count = 0;
  auto cursor = start;
  while (cursor < end) {
    auto next = advance(cursor, scale);
    if (!next.isValid() || next <= cursor) {
      break;
    }
    ++count;
    cursor = next;
  }
  return count;
}

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

    auto next = advance(cursor, scale);
    if (!next.isValid() || next <= cursor) {
      return ChartRangeError::InvalidTime;
    }
    buckets.push_back({cursor, std::min(next, end)});
    cursor = next;
  }
  return ChartRange(scale, std::move(buckets));
}

} // namespace inputcounter
