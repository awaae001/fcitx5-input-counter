// SPDX-License-Identifier: MIT

//! Exercises custom chart range validation and bucketing.

#include "chart_range.h"

#include <stdexcept>
#include <variant>

#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QTimeZone>

namespace {

using inputcounter::ChartRange;
using inputcounter::ChartRangeError;
using inputcounter::ChartScale;

QDateTime utc(int year, int month, int day, int hour, int minute = 0) {
  return QDateTime(QDate(year, month, day), QTime(hour, minute),
                   QTimeZone::UTC);
}

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void rounds_partial_hours_outward() {
  const auto result = ChartRange::create(utc(2026, 8, 8, 10, 15),
                                         utc(2026, 8, 8, 12, 1),
                                         ChartScale::OneHour);
  const auto &range = std::get<ChartRange>(result);

  require(range.start() == utc(2026, 8, 8, 10),
          "start was not rounded down");
  require(range.end() == utc(2026, 8, 8, 13), "end was not rounded up");
  require(range.buckets().size() == 3, "unexpected bucket count");
}

void splits_using_selected_scale() {
  const auto result = ChartRange::create(utc(2026, 8, 8, 0),
                                         utc(2026, 8, 9, 1),
                                         ChartScale::TwelveHours);
  const auto &range = std::get<ChartRange>(result);

  require(range.buckets().size() == 3, "partial final bucket was omitted");
}

void rejects_reversed_range() {
  const auto result = ChartRange::create(utc(2026, 8, 9, 0),
                                         utc(2026, 8, 8, 0),
                                         ChartScale::OneDay);

  require(std::get<ChartRangeError>(result) ==
              ChartRangeError::EndNotAfterStart,
          "reversed range was accepted");
}

void rejects_excessive_bucket_count() {
  const auto start = utc(2026, 1, 1, 0);
  const auto result = ChartRange::create(
      start, start.addSecs(513 * 60 * 60), ChartScale::OneHour);

  require(std::get<ChartRangeError>(result) == ChartRangeError::TooManyBuckets,
          "excessive bucket count was accepted");
}

} // namespace

int main() {
  rounds_partial_hours_outward();
  splits_using_selected_scale();
  rejects_reversed_range();
  rejects_excessive_bucket_count();
}
