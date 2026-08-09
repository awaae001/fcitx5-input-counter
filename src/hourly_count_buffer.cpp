// SPDX-License-Identifier: MIT

//! Implements buffered persistence of hourly character counts.

#include "hourly_count_buffer.h"

namespace inputcounter {

HourlyCountBuffer::HourlyCountBuffer(const std::string &path) : db_(path) {}

void HourlyCountBuffer::add(std::int64_t unixSeconds, std::uint64_t chars) {
  pending_[hourStartOf(unixSeconds)] += chars;
}

void HourlyCountBuffer::flush() {
  while (!pending_.empty()) {
    const auto entry = pending_.begin();
    db_.addChars(entry->first, entry->second);
    pending_.erase(entry);
  }
}

} // namespace inputcounter
