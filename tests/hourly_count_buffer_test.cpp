// SPDX-License-Identifier: MIT

//! Exercises buffered hourly persistence.

#include "hourly_count_buffer.h"

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "stats_db.h"

namespace {

using inputcounter::HourlyCountBuffer;
using inputcounter::StatsDb;

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
  HourlyCountBuffer buffer(database.path());

  buffer.add(3599, 2);
  buffer.add(3601, 3);
  buffer.add(3610, 4);
  buffer.flush();

  StatsDb persisted(database.path());
  const auto rows = persisted.allHourly();
  require(rows.size() == 2, "counts were not split into hourly buckets");
  require(rows[0].hour == 0 && rows[0].chars == 2,
          "first hourly bucket was incorrect");
  require(rows[1].hour == 3600 && rows[1].chars == 7,
          "second hourly bucket was not aggregated");
}

void flushes_each_count_once() {
  TemporaryDatabase database;
  HourlyCountBuffer buffer(database.path());

  buffer.add(3600, 5);
  buffer.flush();
  buffer.flush();

  StatsDb persisted(database.path());
  require(persisted.totalChars() == 5, "a persisted count was flushed twice");
}

} // namespace

int main() {
  aggregates_counts_by_hour();
  flushes_each_count_once();
}
