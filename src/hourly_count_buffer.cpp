// SPDX-License-Identifier: MIT

//! Implements buffered persistence of hourly character counts.

#include "hourly_count_buffer.h"

#include "database_manager.h"
#include "hourly_count.h"

namespace inputcounter {

HourlyCountBuffer::HourlyCountBuffer(DatabaseManager &database) noexcept
    : database_(database) {}

void HourlyCountBuffer::add(std::int64_t unixSeconds, std::uint64_t chars) {
  pending_[hourStartOf(unixSeconds)] += chars;
}

void HourlyCountBuffer::flush() {
  while (!pending_.empty()) {
    const auto entry = pending_.begin();
    database_.addChars(entry->first, entry->second);
    pending_.erase(entry);
  }
}

} // namespace inputcounter
