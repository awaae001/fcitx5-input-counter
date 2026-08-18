// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_DATABASE_MANAGER_H
#define FCITX5_INPUT_COUNTER_DATABASE_MANAGER_H

//! Owns buffered statistics and their process-local SQLite connection.

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "statistics_types.h"

struct sqlite3;

namespace inputcounter
{

  /// Manages pending counts and the process-local statistics database.
  ///
  /// Create one manager at each process composition root and inject references
  /// into database consumers. All operations throw std::runtime_error on
  /// failure. The owner must serialize access; this class does not provide a
  /// connection pool.
  class DatabaseManager final
  {
  public:
    /// Opens the default statistics database, creating its parent directory.
    DatabaseManager();

    /// Opens the statistics database at path.
    explicit DatabaseManager(const std::string &path);

    /// Closes the owned SQLite connection.
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    /// Adds chars to the pending bucket containing unixSeconds.
    void recordChars(std::int64_t unixSeconds, std::uint64_t chars);

    /// Persists all pending hourly buckets.
    ///
    /// Successfully persisted buckets are removed immediately. If persistence
    /// throws, the failed bucket and all later buckets remain pending.
    void flush();

    /// Returns all overview aggregates from one consistent database snapshot.
    StatisticsSummary summary(std::int64_t todayStart,
                              std::int64_t last24HoursStart,
                              std::int64_t last7DaysStart);

    /// Returns hourly rows in the half-open range [start, end), ascending.
    std::vector<HourlyCount> hourlyBetween(std::int64_t start, std::int64_t end);

    /// Deletes all recorded statistics.
    void reset();

  private:
    void persistChars(std::int64_t hourStart, std::uint64_t chars);

    sqlite3 *db_ = nullptr;
    std::map<std::int64_t, std::uint64_t> pending_;
  };

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_DATABASE_MANAGER_H
