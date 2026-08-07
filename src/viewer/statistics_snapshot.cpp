// SPDX-License-Identifier: MIT

//! Implements statistics aggregation for the viewer.

#include "statistics_snapshot.h"

#include <chrono>
#include <ctime>
#include <map>

#include <QDateTime>
#include <QLocale>

#include "../stats_db.h"

namespace inputcounter {

namespace {

using Counts = std::map<std::int64_t, std::uint64_t>;
using Bars = std::vector<std::pair<QString, std::uint64_t>>;

constexpr std::int64_t kHour = 60 * 60;
constexpr std::int64_t kDay = 24 * kHour;
constexpr int kHours = 24;
constexpr int kWeek = 7;
constexpr int kMonth = 30;

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

Counts readHours(StatsDb &db, std::int64_t since) {
  Counts counts;
  for (const auto &row : db.hourlySince(since)) {
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

} // namespace

std::int64_t nowSeconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

StatisticsSnapshot load(StatsDb &db, std::int64_t now) {
  const auto today = dayStart(now);
  const auto total = db.totalChars();
  const auto todayCount = db.charsSince(today);
  const auto last24Hours = db.charsSince(now - kHours * kHour);
  const auto last7Days = db.charsSince(today - (kWeek - 1) * kDay);
  const auto hours = readHours(db, now - kMonth * kDay);
  const auto days = sumDays(hours);

  return {
      total,
      todayCount,
      last24Hours,
      last7Days,
      hourBars(hours, now),
      dayBars(days, now, kWeek),
      dayBars(days, now, kMonth),
  };
}

QString format(std::uint64_t value) {
  return QLocale().toString(static_cast<qulonglong>(value));
}

} // namespace inputcounter
