// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_HOURLY_COUNT_BUFFER_H
#define FCITX5_INPUT_COUNTER_HOURLY_COUNT_BUFFER_H

//! Buffers character counts by hour until they can be persisted.

#include <cstdint>
#include <map>
#include <string>

#include "stats_db.h"

namespace inputcounter {

/// Owns pending hourly counts and the database connection that persists them.
class HourlyCountBuffer final {
public:
  /// Opens the statistics database at path.
  ///
  /// Throws std::runtime_error when the database cannot be opened.
  explicit HourlyCountBuffer(const std::string &path);

  /// Adds chars to the bucket containing unixSeconds.
  void add(std::int64_t unixSeconds, std::uint64_t chars);

  /// Persists pending buckets.
  ///
  /// Successfully persisted buckets are removed immediately. If persistence
  /// throws, the failed bucket and all later buckets remain pending.
  void flush();

private:
  StatsDb db_;
  std::map<std::int64_t, std::uint64_t> pending_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_HOURLY_COUNT_BUFFER_H
