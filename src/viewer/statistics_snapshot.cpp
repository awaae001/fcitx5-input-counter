// SPDX-License-Identifier: MIT

//! Implements statistics aggregation for the viewer.

#include "statistics_snapshot.h"

#include <chrono>
#include <ctime>
#include <map>

#include <QDateTime>
#include <QLocale>
#include <QTime>

#include "../database_manager.h"
#include "chart_range.h"

namespace inputcounter {

namespace {

using Counts = std::map<std::int64_t, std::uint64_t>;
using Bars = std::vector<std::pair<QString, std::uint64_t>>;

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

std::uint64_t at(const Counts &counts, std::int64_t timestamp) {
  const auto it = counts.find(timestamp);
  return it == counts.end() ? 0 : it->second;
}

Counts readAllHours(DatabaseManager &database) {
  Counts counts;
  for (const auto &row : database.allHourly()) {
    counts[row.hour] += row.chars;
  }
  return counts;
}

Counts sumDays(const Counts &hours) {
  Counts days;
  for (const auto &[hour, chars] : hours) {
    days[dayStart(hour)] += chars;
  }
  return days;
}

std::int64_t monthStart(std::int64_t timestamp) {
  const auto date = QDateTime::fromSecsSinceEpoch(timestamp).date();
  return QDateTime(QDate(date.year(), date.month(), 1), QTime(0, 0))
      .toSecsSinceEpoch();
}

Counts sumMonths(const Counts &hours) {
  Counts months;
  for (const auto &[hour, chars] : hours) {
    months[monthStart(hour)] += chars;
  }
  return months;
}

std::int64_t yearStart(std::int64_t timestamp) {
  const auto year = QDateTime::fromSecsSinceEpoch(timestamp).date().year();
  return QDateTime(QDate(year, 1, 1), QTime(0, 0)).toSecsSinceEpoch();
}

Counts sumYears(const Counts &hours) {
  Counts years;
  for (const auto &[hour, chars] : hours) {
    years[yearStart(hour)] += chars;
  }
  return years;
}

Bars hourBars(const Counts &counts, std::int64_t now) {
  Bars bars;
  bars.reserve(kHours);
  for (int offset = kHours - 1; offset >= 0; --offset) {
    const auto hour = hourStartOf(now - offset * kHour);
    bars.emplace_back(
        QDateTime::fromSecsSinceEpoch(hour).toString(QStringLiteral("HH")),
        at(counts, hour));
  }
  return bars;
}

Bars dayBars(const Counts &counts, std::int64_t now, int count) {
  Bars bars;
  bars.reserve(count);
  for (int offset = count - 1; offset >= 0; --offset) {
    const auto day = dayStart(now - offset * kDay);
    bars.emplace_back(
        QDateTime::fromSecsSinceEpoch(day).toString(QStringLiteral("MM-dd")),
        at(counts, day));
  }
  return bars;
}

Bars sixHourBars(const Counts &counts, std::int64_t now) {
  const auto rangeEnd = hourStartOf(now) + kHour;

  Bars bars;
  bars.reserve(kSixHourBars);
  for (int offset = kSixHourBars - 1; offset >= 0; --offset) {
    const auto start = rangeEnd - (offset + 1) * 6 * kHour;
    std::uint64_t total = 0;
    for (int hour = 0; hour < 6; ++hour) {
      total += at(counts, start + hour * kHour);
    }
    bars.emplace_back(QDateTime::fromSecsSinceEpoch(start).toString(
                          QStringLiteral("MM-dd HH:mm")),
                      total);
  }
  return bars;
}

Bars monthBars(const Counts &counts, std::int64_t now) {
  Bars bars;
  bars.reserve(kYearMonths);
  const auto today = QDateTime::fromSecsSinceEpoch(now).date();
  const QDate currentMonth(today.year(), today.month(), 1);
  for (int offset = kYearMonths - 1; offset >= 0; --offset) {
    const auto month = currentMonth.addMonths(-offset);
    const auto start = QDateTime(month, QTime(0, 0)).toSecsSinceEpoch();
    bars.emplace_back(month.toString(QStringLiteral("yyyy-MM")),
                      at(counts, start));
  }
  return bars;
}

Bars yearBars(const Counts &counts) {
  Bars bars;
  bars.reserve(counts.size());
  for (const auto &[start, chars] : counts) {
    bars.emplace_back(
        QDateTime::fromSecsSinceEpoch(start).toString(QStringLiteral("yyyy")),
        chars);
  }
  return bars;
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

} // namespace

std::int64_t nowSeconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

StatisticsSnapshot load(DatabaseManager &database, std::int64_t now) {
  const auto today = dayStart(now);
  const auto total = database.totalChars();
  const auto todayCount = database.charsSince(today);
  const auto last24Hours = database.charsSince(now - kHours * kHour);
  const auto last7Days = database.charsSince(today - (kWeek - 1) * kDay);
  const auto allHours = readAllHours(database);
  const auto days = sumDays(allHours);
  const auto months = sumMonths(allHours);
  const auto years = sumYears(allHours);

  return {
      total,
      todayCount,
      last24Hours,
      last7Days,
      hourBars(allHours, now),
      sixHourBars(allHours, now),
      dayBars(days, now, kMonth),
      monthBars(months, now),
      yearBars(years),
  };
}

ChartBars load(DatabaseManager &database, const ChartRange &range) {
  const auto rows = database.hourlyBetween(range.start().toSecsSinceEpoch(),
                                           range.end().toSecsSinceEpoch());
  ChartBars bars;
  bars.reserve(range.buckets().size());

  auto row = rows.begin();
  for (const auto &bucket : range.buckets()) {
    const auto start = bucket.start.toSecsSinceEpoch();
    const auto end = bucket.end.toSecsSinceEpoch();
    std::uint64_t total = 0;
    while (row != rows.end() && row->hour < start) {
      ++row;
    }
    while (row != rows.end() && row->hour < end) {
      total += row->chars;
      ++row;
    }
    bars.emplace_back(bucketLabel(bucket, range.scale()), total);
  }
  return bars;
}

QString format(std::uint64_t value) {
  return QLocale().toString(static_cast<qulonglong>(value));
}

} // namespace inputcounter
