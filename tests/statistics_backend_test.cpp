// SPDX-License-Identifier: MIT

//! Exercises consistent statistics operations over durable and pending data.

#include "statistics_backend.h"

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "database_manager.h"
namespace {

using inputcounter::DatabaseManager;
using inputcounter::StatisticsBackend;
using inputcounter::TimeRange;

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

class TemporaryDatabase final {
public:
  TemporaryDatabase() {
    auto pattern = (std::filesystem::temp_directory_path() /
                    "fcitx5-input-counter-backend-test-XXXXXX")
                       .string();
    const auto *directory = ::mkdtemp(pattern.data());
    if (directory == nullptr) {
      throw std::runtime_error("could not create temporary test directory");
    }
    directory_ = directory;
  }

  ~TemporaryDatabase() {
    std::error_code error;
    std::filesystem::remove_all(directory_, error);
  }

  TemporaryDatabase(const TemporaryDatabase &) = delete;
  TemporaryDatabase &operator=(const TemporaryDatabase &) = delete;

  std::string path() const { return (directory_ / "stats.db").string(); }

private:
  std::filesystem::path directory_;
};

void includes_pending_counts_in_summary() {
  TemporaryDatabase databasePath;
  DatabaseManager database(databasePath.path());
  StatisticsBackend backend(database);
  database.recordChars(7200, 7);

  const auto summary = backend.summary(0, 3600, 7200);

  require(summary.total == 7, "pending total was omitted");
  require(summary.today == 7, "pending daily count was omitted");
  require(summary.last24Hours == 7, "pending rolling count was omitted");
  require(summary.last7Days == 7, "pending weekly count was omitted");
  require(summary.hasData && summary.firstHour == 7200,
          "first persisted hour was incorrect");
}

void sums_ordered_buckets() {
  TemporaryDatabase databasePath;
  DatabaseManager database(databasePath.path());
  StatisticsBackend backend(database);
  database.recordChars(0, 2);
  database.recordChars(3600, 3);
  database.recordChars(7200, 5);

  const auto counts =
      backend.bucketCounts(std::vector<TimeRange>{{0, 7200}, {7200, 10800}});

  require(counts == std::vector<std::uint64_t>({5, 5}),
          "bucket totals were incorrect");
}

void sums_independent_summary_windows() {
  TemporaryDatabase databasePath;
  DatabaseManager database(databasePath.path());
  StatisticsBackend backend(database);
  database.recordChars(0, 2);
  database.recordChars(3600, 3);
  database.recordChars(7200, 5);

  const auto summary = backend.summary(7200, 3600, 0);

  require(summary.total == 10, "total summary count was incorrect");
  require(summary.today == 5, "daily summary window was incorrect");
  require(summary.last24Hours == 8,
          "rolling summary window was incorrect");
  require(summary.last7Days == 10, "weekly summary window was incorrect");
}

void reports_empty_summary() {
  TemporaryDatabase databasePath;
  DatabaseManager database(databasePath.path());
  StatisticsBackend backend(database);

  const auto summary = backend.summary(0, 0, 0);

  require(summary.total == 0, "empty summary had a nonzero total");
  require(!summary.hasData, "empty summary reported stored data");
}

void rejects_overlapping_buckets() {
  TemporaryDatabase databasePath;
  DatabaseManager database(databasePath.path());
  StatisticsBackend backend(database);

  bool rejected = false;
  try {
    backend.bucketCounts(std::vector<TimeRange>{{0, 7200}, {3600, 10800}});
  } catch (const std::invalid_argument &) {
    rejected = true;
  }

  require(rejected, "overlapping buckets were accepted");
}

void discards_pending_counts_on_reset() {
  TemporaryDatabase databasePath;
  DatabaseManager database(databasePath.path());
  StatisticsBackend backend(database);
  database.recordChars(0, 2);
  database.flush();
  database.recordChars(3600, 3);

  backend.reset();
  database.flush();

  require(database.summary(0, 0, 0).total == 0,
          "pending counts were restored after reset");
}

} // namespace

int main() {
  includes_pending_counts_in_summary();
  sums_ordered_buckets();
  sums_independent_summary_windows();
  reports_empty_summary();
  rejects_overlapping_buckets();
  discards_pending_counts_on_reset();
}
