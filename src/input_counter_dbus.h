// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_INPUT_COUNTER_DBUS_H
#define FCITX5_INPUT_COUNTER_INPUT_COUNTER_DBUS_H

//! Declares the addon's versioned D-Bus statistics interface.

#include <cstdint>
#include <tuple>
#include <vector>

#include <fcitx-utils/dbus/objectvtable.h>

namespace inputcounter {

class StatisticsBackend;

/// Exposes plugin-owned statistics through the Fcitx session bus.
class InputCounterDBus final
    : public fcitx::dbus::ObjectVTable<InputCounterDBus> {
public:
  /// Borrows backend for this object's lifetime.
  explicit InputCounterDBus(StatisticsBackend &backend) noexcept;

private:
  using Bucket = fcitx::dbus::DBusStruct<std::int64_t, std::int64_t>;
  using Summary = std::tuple<std::uint64_t, std::uint64_t, std::uint64_t,
                             std::uint64_t, bool, std::int64_t>;

  Summary getSummary(std::int64_t todayStart, std::int64_t last24HoursStart,
                     std::int64_t last7DaysStart);
  std::vector<std::uint64_t>
  getBucketCounts(const std::vector<Bucket> &buckets);
  void reset();

  FCITX_OBJECT_VTABLE_METHOD(getSummary, "GetSummary", "xxx", "ttttbx");
  FCITX_OBJECT_VTABLE_METHOD(getBucketCounts, "GetBucketCounts", "a(xx)", "at");
  FCITX_OBJECT_VTABLE_METHOD(reset, "Reset", "", "");

  StatisticsBackend &backend_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_INPUT_COUNTER_DBUS_H
