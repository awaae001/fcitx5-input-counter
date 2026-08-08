// SPDX-License-Identifier: MIT

//! Implements the shared SQLite storage for hourly character counts.

#include "stats_db.h"

#include <sqlite3.h>

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "embedded_sql.h"

namespace inputcounter {

namespace {

/// RAII wrapper for a prepared SQLite statement.
class Statement {
public:
  Statement(sqlite3 *db, const char *sql) : db_(db) {
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt_, nullptr) != SQLITE_OK) {
      throw std::runtime_error(sqlite3_errmsg(db_));
    }
  }
  ~Statement() { sqlite3_finalize(stmt_); }

  Statement(const Statement &) = delete;
  Statement &operator=(const Statement &) = delete;

  void bindInt64(int index, std::int64_t value) {
    sqlite3_bind_int64(stmt_, index, static_cast<sqlite3_int64>(value));
  }

  /// Steps once, expecting SQLITE_DONE.
  void stepDone() {
    if (sqlite3_step(stmt_) != SQLITE_DONE) {
      throw std::runtime_error(sqlite3_errmsg(db_));
    }
  }

  /// Steps once, returning whether a row is available.
  bool stepRow() {
    const int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) {
      return true;
    }
    if (rc != SQLITE_DONE) {
      throw std::runtime_error(sqlite3_errmsg(db_));
    }
    return false;
  }

  std::int64_t columnInt64(int index) const {
    return static_cast<std::int64_t>(sqlite3_column_int64(stmt_, index));
  }

private:
  sqlite3 *db_;
  sqlite3_stmt *stmt_ = nullptr;
};

void execSql(sqlite3 *db, const char *sql) {
  char *error = nullptr;
  if (sqlite3_exec(db, sql, nullptr, nullptr, &error) != SQLITE_OK) {
    std::string message = error != nullptr ? error : "unknown SQLite error";
    sqlite3_free(error);
    throw std::runtime_error(std::move(message));
  }
}

} // namespace

std::string statsDatabasePath() {
  std::filesystem::path base;
  if (const char *xdgDataHome = std::getenv("XDG_DATA_HOME");
      xdgDataHome != nullptr && *xdgDataHome != '\0') {
    base = xdgDataHome;
  } else if (const char *home = std::getenv("HOME");
             home != nullptr && *home != '\0') {
    base = std::filesystem::path(home) / ".local" / "share";
  } else {
    throw std::runtime_error("neither XDG_DATA_HOME nor HOME is set");
  }

  base /= "fcitx5/input-counter";
  std::error_code ec;
  std::filesystem::create_directories(base, ec);
  return (base / "stats.db").string();
}

StatsDb::StatsDb(const std::string &path) {
  if (sqlite3_open_v2(path.c_str(), &db_,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                          SQLITE_OPEN_FULLMUTEX,
                      nullptr) != SQLITE_OK) {
    const std::string message = db_ != nullptr ? sqlite3_errmsg(db_) : "out of memory";
    sqlite3_close(db_);
    db_ = nullptr;
    throw std::runtime_error(message);
  }

  try {
    execSql(db_, "PRAGMA journal_mode = WAL");
    execSql(db_, "PRAGMA busy_timeout = 2000");
    execSql(db_, kSchemaSql);
  } catch (...) {
    sqlite3_close(db_);
    db_ = nullptr;
    throw;
  }
}

StatsDb::~StatsDb() {
  if (db_ != nullptr) {
    sqlite3_close(db_);
  }
}

void StatsDb::addChars(std::int64_t hourStart, std::uint64_t chars) {
  if (chars == 0) {
    return;
  }
  Statement stmt(db_,
                 "INSERT INTO stats(hour, chars) VALUES(?1, ?2) "
                 "ON CONFLICT(hour) DO UPDATE SET chars = chars + excluded.chars");
  stmt.bindInt64(1, hourStart);
  stmt.bindInt64(2, static_cast<std::int64_t>(chars));
  stmt.stepDone();
}

std::uint64_t StatsDb::totalChars() {
  Statement stmt(db_, "SELECT COALESCE(SUM(chars), 0) FROM stats");
  stmt.stepRow();
  return static_cast<std::uint64_t>(stmt.columnInt64(0));
}

std::uint64_t StatsDb::charsSince(std::int64_t since) {
  Statement stmt(
      db_, "SELECT COALESCE(SUM(chars), 0) FROM stats WHERE hour >= ?1");
  stmt.bindInt64(1, hourStartOf(since));
  stmt.stepRow();
  return static_cast<std::uint64_t>(stmt.columnInt64(0));
}

std::vector<HourlyCount> StatsDb::hourlySince(std::int64_t since) {
  Statement stmt(
      db_, "SELECT hour, chars FROM stats WHERE hour >= ?1 ORDER BY hour");
  stmt.bindInt64(1, hourStartOf(since));
  std::vector<HourlyCount> rows;
  while (stmt.stepRow()) {
    rows.push_back(
        {stmt.columnInt64(0), static_cast<std::uint64_t>(stmt.columnInt64(1))});
  }
  return rows;
}

std::vector<HourlyCount> StatsDb::hourlyBetween(std::int64_t start,
                                                std::int64_t end) {
  Statement stmt(db_,
                 "SELECT hour, chars FROM stats "
                 "WHERE hour >= ?1 AND hour < ?2 ORDER BY hour");
  stmt.bindInt64(1, start);
  stmt.bindInt64(2, end);
  std::vector<HourlyCount> rows;
  while (stmt.stepRow()) {
    rows.push_back(
        {stmt.columnInt64(0), static_cast<std::uint64_t>(stmt.columnInt64(1))});
  }
  return rows;
}

std::vector<HourlyCount> StatsDb::allHourly() {
  Statement stmt(db_, "SELECT hour, chars FROM stats ORDER BY hour");
  std::vector<HourlyCount> rows;
  while (stmt.stepRow()) {
    rows.push_back(
        {stmt.columnInt64(0), static_cast<std::uint64_t>(stmt.columnInt64(1))});
  }
  return rows;
}

void StatsDb::reset() { execSql(db_, "DELETE FROM stats"); }

} // namespace inputcounter
