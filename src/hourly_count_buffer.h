// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_HOURLY_COUNT_BUFFER_H
#define FCITX5_INPUT_COUNTER_HOURLY_COUNT_BUFFER_H

//! Buffers character counts by hour until they can be persisted.

#include <cstdint>
#include <map>

namespace inputcounter {

class DatabaseManager;

/// Owns pending hourly counts and persists them through a borrowed manager.
class HourlyCountBuffer final {
public:
  /// Borrows database for the lifetime of this buffer.
  explicit HourlyCountBuffer(DatabaseManager &database) noexcept;

  /// Adds chars to the bucket containing unixSeconds.
  void add(std::int64_t unixSeconds, std::uint64_t chars);

  /// Persists pending buckets.
  ///
  /// Successfully persisted buckets are removed immediately. If persistence
  /// throws, the failed bucket and all later buckets remain pending.
  void flush();

  /// Discards all counts that have not been persisted yet.
  void clear() noexcept;

private:
  DatabaseManager &database_;
  std::map<std::int64_t, std::uint64_t> pending_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_HOURLY_COUNT_BUFFER_H
