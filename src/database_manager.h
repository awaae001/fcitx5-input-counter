// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_DATABASE_MANAGER_H
#define FCITX5_INPUT_COUNTER_DATABASE_MANAGER_H

//! Owns the process-local SQLite connection used for input statistics.

#include <cstdint>
#include <string>
#include <vector>

#include "hourly_count.h"
#include "statistics_types.h"

struct sqlite3;

namespace inputcounter
{

  /// Manages the process-local connection to the statistics database.
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

    /// Adds chars to the row for the given hour-start timestamp.
    void addChars(std::int64_t hourStart, std::uint64_t chars);

    /// Returns all overview aggregates from one consistent database snapshot.
    StatisticsSummary summary(std::int64_t todayStart,
                              std::int64_t last24HoursStart,
                              std::int64_t last7DaysStart);

    /// Returns hourly rows in the half-open range [start, end), ascending.
    std::vector<HourlyCount> hourlyBetween(std::int64_t start, std::int64_t end);

    /// Deletes all recorded statistics.
    void reset();

  private:
    sqlite3 *db_ = nullptr;
  };

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_DATABASE_MANAGER_H
