// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_STATISTICS_BACKEND_H
#define FCITX5_INPUT_COUNTER_STATISTICS_BACKEND_H

//! Owns consistent statistics operations exposed by the addon.

#include <cstdint>
#include <vector>

#include "statistics_types.h"

namespace inputcounter
{

  class DatabaseManager;
  /// Provides consistent statistics operations over one database manager.
  class StatisticsBackend final
  {
  public:
    /// Borrows database for this backend's lifetime.
    explicit StatisticsBackend(DatabaseManager &database) noexcept;

    /// Flushes pending counts and returns overview aggregates.
    StatisticsSummary summary(std::int64_t todayStart,
                              std::int64_t last24HoursStart,
                              std::int64_t last7DaysStart);

    /// Flushes pending counts and sums each ordered, non-overlapping range.
    ///
    /// At most kMaximumStatisticsBuckets ranges are accepted. Throws
    /// std::invalid_argument if a range is empty, reversed, or overlaps its
    /// predecessor.
    std::vector<std::uint64_t> bucketCounts(const std::vector<TimeRange> &ranges);

    /// Deletes persisted and pending statistics.
    void reset();

  private:
    DatabaseManager &database_;
  };

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_STATISTICS_BACKEND_H
