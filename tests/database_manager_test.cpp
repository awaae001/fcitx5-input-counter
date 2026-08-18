// SPDX-License-Identifier: MIT

//! Exercises buffered database management.

#include "database_manager.h"

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {

using inputcounter::DatabaseManager;
using inputcounter::hourStartOf;

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

class TemporaryDatabase final {
public:
  TemporaryDatabase() {
    auto pattern = (std::filesystem::temp_directory_path() /
                    "fcitx5-input-counter-test-XXXXXX")
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

void aggregates_counts_by_hour() {
  TemporaryDatabase database;
  DatabaseManager manager(database.path());

  manager.recordChars(3599, 2);
  manager.recordChars(3601, 3);
  manager.recordChars(3610, 4);
  manager.flush();

  const auto rows = manager.hourlyBetween(0, 2 * 60 * 60);
  require(rows.size() == 2, "counts were not split into hourly buckets");
  require(rows[0].hour == 0 && rows[0].chars == 2,
          "first hourly bucket was incorrect");
  require(rows[1].hour == 3600 && rows[1].chars == 7,
          "second hourly bucket was not aggregated");
}

void flushes_each_count_once() {
  TemporaryDatabase database;
  DatabaseManager manager(database.path());

  manager.recordChars(3600, 5);
  manager.flush();
  manager.flush();

  require(manager.summary(0, 0, 0).total == 5,
          "a persisted count was flushed twice");
}

void ignores_zero_counts() {
  TemporaryDatabase database;
  DatabaseManager manager(database.path());

  manager.recordChars(3600, 0);
  manager.flush();

  require(manager.hourlyBetween(0, 2 * 60 * 60).empty(),
          "zero count created an hourly bucket");
}

void rounds_pre_epoch_timestamps_down() {
  require(hourStartOf(-1) == -3600,
          "pre-epoch timestamp was rounded toward zero");
  require(hourStartOf(-3600) == -3600,
          "aligned pre-epoch timestamp changed hour");
}

} // namespace

int main() {
  aggregates_counts_by_hour();
  flushes_each_count_once();
  ignores_zero_counts();
  rounds_pre_epoch_timestamps_down();
}
