// SPDX-License-Identifier: MIT

//! Implements consistent statistics operations exposed by the addon.

#include "statistics_backend.h"
#include "database_manager.h"

#include <stdexcept>

namespace inputcounter
{

  StatisticsSummary StatisticsBackend::summary(std::int64_t todayStart,
                                               std::int64_t last24HoursStart,
                                               std::int64_t last7DaysStart)
  {
    database_.flush();
    return database_.summary(todayStart, last24HoursStart, last7DaysStart);
  }

  std::vector<std::uint64_t>
  StatisticsBackend::bucketCounts(const std::vector<TimeRange> &ranges)
  {
    if (ranges.size() > kMaximumStatisticsBuckets)
    {
      throw std::invalid_argument("too many statistics buckets");
    }
    if (ranges.empty())
    {
      return {};
    }

    database_.flush();
    const auto rows = database_.hourlyBetween(ranges.front().start, ranges.back().end);
    std::vector<std::uint64_t> result;
    result.reserve(ranges.size());

    auto row = rows.begin();
    for (const auto &range : ranges)
    {
      while (row != rows.end() && row->hour < range.start)
      {
        ++row;
      }

      std::uint64_t total = 0;
      while (row != rows.end() && row->hour < range.end)
      {
        total += row->chars;
        ++row;
      }
      result.push_back(total);
    }
    return result;
  }

  void StatisticsBackend::reset()
  {
    database_.reset();
  }

} // namespace inputcounter
