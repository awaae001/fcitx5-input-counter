// SPDX-License-Identifier: MIT

//! Implements consistent statistics operations exposed by the addon.

#include "statistics_backend.h"

#include <cstddef>
#include <stdexcept>

#include "database_manager.h"
#include "hourly_count_buffer.h"

namespace inputcounter {

constexpr std::size_t kMaximumBuckets = 512;

StatisticsBackend::StatisticsBackend(DatabaseManager &database,
                                     HourlyCountBuffer &pendingCounts) noexcept
    : database_(database), pendingCounts_(pendingCounts) {}

StatisticsSummary StatisticsBackend::summary(std::int64_t todayStart,
                                             std::int64_t last24HoursStart,
                                             std::int64_t last7DaysStart) {
  pendingCounts_.flush();
  const auto firstHour = database_.firstHour();
  return {
      database_.totalChars(),
      database_.charsSince(todayStart),
      database_.charsSince(last24HoursStart),
      database_.charsSince(last7DaysStart),
      firstHour.has_value(),
      firstHour.value_or(0),
  };
}

std::vector<std::uint64_t>
StatisticsBackend::bucketCounts(const std::vector<TimeRange> &ranges) {
  if (ranges.size() > kMaximumBuckets) {
    throw std::invalid_argument("too many statistics buckets");
  }
  for (std::size_t index = 0; index < ranges.size(); ++index) {
    const auto &range = ranges[index];
    if (range.start >= range.end) {
      throw std::invalid_argument("statistics bucket is empty or reversed");
    }
    if (index > 0 && ranges[index - 1].end > range.start) {
      throw std::invalid_argument("statistics buckets overlap");
    }
  }
  if (ranges.empty()) {
    return {};
  }

  pendingCounts_.flush();
  const auto rows =
      database_.hourlyBetween(ranges.front().start, ranges.back().end);
  std::vector<std::uint64_t> result;
  result.reserve(ranges.size());

  auto row = rows.begin();
  for (const auto &range : ranges) {
    while (row != rows.end() && row->hour < range.start) {
      ++row;
    }

    std::uint64_t total = 0;
    while (row != rows.end() && row->hour < range.end) {
      total += row->chars;
      ++row;
    }
    result.push_back(total);
  }
  return result;
}

void StatisticsBackend::reset() {
  // Delete durable state first so a failed DELETE leaves pending data intact.
  database_.reset();
  pendingCounts_.clear();
}

} // namespace inputcounter
