// SPDX-License-Identifier: MIT

//! Implements bounded statistics queries and chart value formatting.

#include "statistics_snapshot.h"

#include <chrono>
#include <ctime>
#include <stdexcept>
#include <utility>

#include <QDate>
#include <QDateTime>
#include <QLocale>
#include <QTime>

#include "../hourly_count.h"
#include "chart_range.h"

namespace inputcounter {

namespace {

constexpr std::int64_t kHour = 60 * 60;
constexpr std::int64_t kDay = 24 * kHour;
constexpr int kHours = 24;
constexpr int kWeek = 7;
constexpr int kSixHourBars = 4 * kWeek;
constexpr int kMonth = 30;
constexpr int kYearMonths = 12;

std::int64_t dayStart(std::int64_t timestamp) {
  const auto time = static_cast<std::time_t>(timestamp);
  std::tm local{};
  localtime_r(&time, &local);
  local.tm_hour = 0;
  local.tm_min = 0;
  local.tm_sec = 0;
  return static_cast<std::int64_t>(std::mktime(&local));
}

QString bucketLabel(const TimeBucket &bucket, ChartScale scale) {
  switch (scale) {
  case ChartScale::OneHour:
  case ChartScale::SixHours:
  case ChartScale::TwelveHours:
    return bucket.start.toString(QStringLiteral("MM-dd HH:mm"));
  case ChartScale::OneDay:
  case ChartScale::OneWeek:
    return bucket.start.toString(QStringLiteral("yyyy-MM-dd"));
  case ChartScale::OneMonth:
    return bucket.start.toString(QStringLiteral("yyyy-MM"));
  }
  return {};
}

ChartBucket dateBucket(const QDate &date, QString format) {
  const QDateTime start(date, QTime(0, 0));
  const QDateTime end(date.addDays(1), QTime(0, 0));
  return {{start.toSecsSinceEpoch(), end.toSecsSinceEpoch()},
          start.toString(std::move(format))};
}

} // namespace

std::int64_t nowSeconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

SummaryQuery summaryQuery(std::int64_t now) {
  const auto today = dayStart(now);
  return {today, hourStartOf(now - kHours * kHour), today - (kWeek - 1) * kDay};
}

ChartQuery last24HoursQuery(std::int64_t now) {
  const auto end = hourStartOf(now) + kHour;
  ChartQuery result;
  result.reserve(kHours);
  for (int offset = kHours - 1; offset >= 0; --offset) {
    const auto start = end - (offset + 1) * kHour;
    result.push_back({
        {start, start + kHour},
        QDateTime::fromSecsSinceEpoch(start).toString(QStringLiteral("HH")),
    });
  }
  return result;
}

ChartQuery last7DaysQuery(std::int64_t now) {
  const auto end = hourStartOf(now) + kHour;
  ChartQuery result;
  result.reserve(kSixHourBars);
  for (int offset = kSixHourBars - 1; offset >= 0; --offset) {
    const auto start = end - (offset + 1) * 6 * kHour;
    result.push_back({
        {start, start + 6 * kHour},
        QDateTime::fromSecsSinceEpoch(start).toString(
            QStringLiteral("MM-dd HH:mm")),
    });
  }
  return result;
}

ChartQuery last30DaysQuery(std::int64_t now) {
  const auto today = QDateTime::fromSecsSinceEpoch(now).date();
  ChartQuery result;
  result.reserve(kMonth);
  for (int offset = kMonth - 1; offset >= 0; --offset) {
    result.push_back(
        dateBucket(today.addDays(-offset), QStringLiteral("MM-dd")));
  }
  return result;
}

ChartQuery last12MonthsQuery(std::int64_t now) {
  const auto today = QDateTime::fromSecsSinceEpoch(now).date();
  const QDate currentMonth(today.year(), today.month(), 1);
  ChartQuery result;
  result.reserve(kYearMonths);
  for (int offset = kYearMonths - 1; offset >= 0; --offset) {
    const auto month = currentMonth.addMonths(-offset);
    const QDateTime start(month, QTime(0, 0));
    const QDateTime end(month.addMonths(1), QTime(0, 0));
    result.push_back({
        {start.toSecsSinceEpoch(), end.toSecsSinceEpoch()},
        month.toString(QStringLiteral("yyyy-MM")),
    });
  }
  return result;
}

ChartQuery allTimeQuery(std::int64_t firstHour, std::int64_t now) {
  const auto firstYear = QDateTime::fromSecsSinceEpoch(firstHour).date().year();
  const auto currentYear = QDateTime::fromSecsSinceEpoch(now).date().year();
  ChartQuery result;
  if (firstYear > currentYear) {
    return result;
  }
  result.reserve(static_cast<std::size_t>(currentYear - firstYear + 1));
  for (int year = firstYear; year <= currentYear; ++year) {
    const QDateTime start(QDate(year, 1, 1), QTime(0, 0));
    const QDateTime end(QDate(year + 1, 1, 1), QTime(0, 0));
    result.push_back({
        {start.toSecsSinceEpoch(), end.toSecsSinceEpoch()},
        QString::number(year),
    });
  }
  return result;
}

ChartQuery customQuery(const ChartRange &range) {
  ChartQuery result;
  result.reserve(range.buckets().size());
  for (const auto &bucket : range.buckets()) {
    result.push_back({
        {bucket.start.toSecsSinceEpoch(), bucket.end.toSecsSinceEpoch()},
        bucketLabel(bucket, range.scale()),
    });
  }
  return result;
}

std::vector<TimeRange> ranges(const ChartQuery &query) {
  std::vector<TimeRange> result;
  result.reserve(query.size());
  for (const auto &bucket : query) {
    result.push_back(bucket.range);
  }
  return result;
}

ChartBars makeBars(const ChartQuery &query,
                   const std::vector<std::uint64_t> &counts) {
  if (query.size() != counts.size()) {
    throw std::invalid_argument("statistics bucket count mismatch");
  }

  ChartBars result;
  result.reserve(query.size());
  for (std::size_t index = 0; index < query.size(); ++index) {
    result.emplace_back(query[index].label, counts[index]);
  }
  return result;
}

QString format(std::uint64_t value) {
  return QLocale().toString(static_cast<qulonglong>(value));
}

} // namespace inputcounter
