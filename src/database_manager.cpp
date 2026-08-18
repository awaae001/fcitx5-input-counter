// SPDX-License-Identifier: MIT

//! Implements process-local management of the statistics database.

#include "database_manager.h"

#include <sqlite3.h>

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <system_error>

#include "embedded_sql.h"

namespace inputcounter
{

  namespace
  {

    /// RAII wrapper for a prepared SQLite statement.
    class Statement
    {
    public:
      Statement(sqlite3 *db, const char *sql) : db_(db)
      {
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt_, nullptr) != SQLITE_OK)
        {
          throw std::runtime_error(sqlite3_errmsg(db_));
        }
      }
      ~Statement() { sqlite3_finalize(stmt_); }

      Statement(const Statement &) = delete;
      Statement &operator=(const Statement &) = delete;

      void bindInt64(int index, std::int64_t value)
      {
        if (sqlite3_bind_int64(stmt_, index, static_cast<sqlite3_int64>(value)) !=
            SQLITE_OK)
        {
          throw std::runtime_error(sqlite3_errmsg(db_));
        }
      }

      /// Steps once, expecting SQLITE_DONE.
      void stepDone()
      {
        if (sqlite3_step(stmt_) != SQLITE_DONE)
        {
          throw std::runtime_error(sqlite3_errmsg(db_));
        }
      }

      /// Steps once, returning whether a row is available.
      bool stepRow()
      {
        const int rc = sqlite3_step(stmt_);
        if (rc == SQLITE_ROW)
        {
          return true;
        }
        if (rc != SQLITE_DONE)
        {
          throw std::runtime_error(sqlite3_errmsg(db_));
        }
        return false;
      }

      std::int64_t columnInt64(int index) const
      {
        return static_cast<std::int64_t>(sqlite3_column_int64(stmt_, index));
      }

      bool columnIsNull(int index) const
      {
        return sqlite3_column_type(stmt_, index) == SQLITE_NULL;
      }

    private:
      sqlite3 *db_;
      sqlite3_stmt *stmt_ = nullptr;
    };

    void execSql(sqlite3 *db, const char *sql)
    {
      char *error = nullptr;
      if (sqlite3_exec(db, sql, nullptr, nullptr, &error) != SQLITE_OK)
      {
        std::string message = error != nullptr ? error : "unknown SQLite error";
        sqlite3_free(error);
        throw std::runtime_error(message);
      }
    }

    std::string defaultDatabasePath()
    {
      std::filesystem::path base;
      if (const char *xdgDataHome = std::getenv("XDG_DATA_HOME");
          xdgDataHome != nullptr && *xdgDataHome != '\0')
      {
        base = xdgDataHome;
      }
      else if (const char *home = std::getenv("HOME");
               home != nullptr && *home != '\0')
      {
        base = std::filesystem::path(home) / ".local" / "share";
      }
      else
      {
        throw std::runtime_error("neither XDG_DATA_HOME nor HOME is set");
      }

      base /= "fcitx5/input-counter";
      std::error_code ec;
      std::filesystem::create_directories(base, ec);
      if (ec)
      {
        throw std::filesystem::filesystem_error(
            "could not create statistics directory", base, ec);
      }
      return (base / "stats.db").string();
    }

  } // namespace

  DatabaseManager::DatabaseManager() : DatabaseManager(defaultDatabasePath()) {}

  DatabaseManager::DatabaseManager(const std::string &path)
  {
    if (sqlite3_open_v2(path.c_str(), &db_,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                            SQLITE_OPEN_NOMUTEX,
                        nullptr) != SQLITE_OK)
    {
      const std::string message =
          db_ != nullptr ? sqlite3_errmsg(db_) : "out of memory";
      sqlite3_close(db_);
      db_ = nullptr;
      throw std::runtime_error(message);
    }

    try
    {
      execSql(db_, "PRAGMA journal_mode = WAL");
      execSql(db_, "PRAGMA busy_timeout = 2000");
      execSql(db_, kSchemaSql);
    }
    catch (...)
    {
      sqlite3_close(db_);
      db_ = nullptr;
      throw;
    }
  }

  DatabaseManager::~DatabaseManager()
  {
    if (db_ != nullptr)
    {
      sqlite3_close(db_);
    }
  }

  void DatabaseManager::recordChars(std::int64_t unixSeconds,
                                    std::uint64_t chars)
  {
    if (chars == 0)
    {
      return;
    }
    pending_[hourStartOf(unixSeconds)] += chars;
  }

  void DatabaseManager::flush()
  {
    while (!pending_.empty())
    {
      const auto entry = pending_.begin();
      persistChars(entry->first, entry->second);
      pending_.erase(entry);
    }
  }

  void DatabaseManager::persistChars(std::int64_t hourStart,
                                     std::uint64_t chars)
  {
    Statement stmt(
        db_, "INSERT INTO stats(hour, chars) VALUES(?1, ?2) "
             "ON CONFLICT(hour) DO UPDATE SET chars = chars + excluded.chars");
    stmt.bindInt64(1, hourStart);
    stmt.bindInt64(2, static_cast<std::int64_t>(chars));
    stmt.stepDone();
  }

  StatisticsSummary DatabaseManager::summary(std::int64_t todayStart,
                                             std::int64_t last24HoursStart,
                                             std::int64_t last7DaysStart)
  {
    Statement stmt(
        db_, "SELECT COALESCE(SUM(chars), 0), "
             "COALESCE(SUM(chars) FILTER (WHERE hour >= ?1), 0), "
             "COALESCE(SUM(chars) FILTER (WHERE hour >= ?2), 0), "
             "COALESCE(SUM(chars) FILTER (WHERE hour >= ?3), 0), MIN(hour) "
             "FROM stats");
    stmt.bindInt64(1, hourStartOf(todayStart));
    stmt.bindInt64(2, hourStartOf(last24HoursStart));
    stmt.bindInt64(3, hourStartOf(last7DaysStart));
    stmt.stepRow();
    const bool hasData = !stmt.columnIsNull(4);
    return {
        static_cast<std::uint64_t>(stmt.columnInt64(0)),
        static_cast<std::uint64_t>(stmt.columnInt64(1)),
        static_cast<std::uint64_t>(stmt.columnInt64(2)),
        static_cast<std::uint64_t>(stmt.columnInt64(3)),
        hasData,
        hasData ? stmt.columnInt64(4) : 0,
    };
  }

  std::vector<HourlyCount> DatabaseManager::hourlyBetween(std::int64_t start,
                                                          std::int64_t end)
  {
    Statement stmt(db_, "SELECT hour, chars FROM stats "
                        "WHERE hour >= ?1 AND hour < ?2 ORDER BY hour");
    stmt.bindInt64(1, start);
    stmt.bindInt64(2, end);
    std::vector<HourlyCount> rows;
    while (stmt.stepRow())
    {
      rows.push_back(
          {stmt.columnInt64(0), static_cast<std::uint64_t>(stmt.columnInt64(1))});
    }
    return rows;
  }

  void DatabaseManager::reset()
  {
    // Delete durable state first so a failed DELETE leaves pending data intact.
    execSql(db_, "DELETE FROM stats");
    pending_.clear();
  }

} // namespace inputcounter
