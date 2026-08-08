// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_STATS_DB_H
#define FCITX5_INPUT_COUNTER_STATS_DB_H

//! Shared SQLite storage for hourly character counts.

#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;

namespace inputcounter {

/// Returns the statistics database path, creating its parent directory.
std::string statsDatabasePath();

/// Rounds a Unix timestamp down to the start of its hour.
constexpr std::int64_t hourStartOf(std::int64_t unixSeconds) {
  return unixSeconds - unixSeconds % 3600;
}

/// One hourly row of the statistics table.
struct HourlyCount {
  std::int64_t hour;
  std::uint64_t chars;
};

/// Owns a SQLite connection to the statistics database.
///
/// All operations throw std::runtime_error on failure.
class StatsDb {
public:
  explicit StatsDb(const std::string &path);
  ~StatsDb();

  StatsDb(const StatsDb &) = delete;
  StatsDb &operator=(const StatsDb &) = delete;

  /// Adds chars to the row for the given hour-start timestamp.
  void addChars(std::int64_t hourStart, std::uint64_t chars);
  /// Returns the sum over all recorded hours.
  std::uint64_t totalChars();
  /// Returns the sum over hours starting at or after the hour of since.
  std::uint64_t charsSince(std::int64_t since);
  /// Returns hourly rows starting at or after the hour of since, ascending.
  std::vector<HourlyCount> hourlySince(std::int64_t since);
  /// Returns hourly rows in the half-open range [start, end), ascending.
  std::vector<HourlyCount> hourlyBetween(std::int64_t start,
                                        std::int64_t end);
  /// Returns every hourly row, ascending.
  std::vector<HourlyCount> allHourly();
  /// Deletes all recorded statistics.
  void reset();

private:
  sqlite3 *db_ = nullptr;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_STATS_DB_H
