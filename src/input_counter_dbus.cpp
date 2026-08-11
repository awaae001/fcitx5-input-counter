// SPDX-License-Identifier: MIT

//! Implements the addon's versioned D-Bus statistics interface.

#include "input_counter_dbus.h"

#include <exception>
#include <stdexcept>
#include <utility>

#include <fcitx-utils/dbus/objectvtable.h>

#include "statistics_backend.h"

namespace inputcounter {

namespace {

constexpr char kUnavailableError[] =
    "org.fcitx.Fcitx.InputCounter1.Error.DataUnavailable";
constexpr char kInvalidArgumentsError[] =
    "org.freedesktop.DBus.Error.InvalidArgs";

template <typename Function> auto translateErrors(Function &&function) {
  try {
    return function();
  } catch (const std::invalid_argument &error) {
    throw fcitx::dbus::MethodCallError(kInvalidArgumentsError, error.what());
  } catch (const std::exception &error) {
    throw fcitx::dbus::MethodCallError(kUnavailableError, error.what());
  }
}

} // namespace

InputCounterDBus::InputCounterDBus(StatisticsBackend *backend) noexcept
    : backend_(backend) {}

std::vector<InputCounterDBus::HourlyValue> InputCounterDBus::getData() {
  return translateErrors([this] {
    const auto rows = requireBackend().data();
    std::vector<HourlyValue> result;
    result.reserve(rows.size());
    for (const auto &row : rows) {
      result.emplace_back(row.hour, row.chars);
    }
    return result;
  });
}

InputCounterDBus::Summary
InputCounterDBus::getSummary(std::int64_t todayStart,
                             std::int64_t last24HoursStart,
                             std::int64_t last7DaysStart) {
  return translateErrors([this, todayStart, last24HoursStart, last7DaysStart] {
    const auto result =
        requireBackend().summary(todayStart, last24HoursStart, last7DaysStart);
    return Summary{result.total,     result.today,   result.last24Hours,
                   result.last7Days, result.hasData, result.firstHour};
  });
}

std::vector<std::uint64_t>
InputCounterDBus::getBucketCounts(const std::vector<Bucket> &buckets) {
  return translateErrors([this, &buckets] {
    std::vector<TimeRange> ranges;
    ranges.reserve(buckets.size());
    for (const auto &bucket : buckets) {
      const auto &[start, end] = bucket.data();
      ranges.push_back({start, end});
    }
    return requireBackend().bucketCounts(ranges);
  });
}

void InputCounterDBus::reset() {
  translateErrors([this] { requireBackend().reset(); });
}

StatisticsBackend &InputCounterDBus::requireBackend() const {
  if (backend_ == nullptr) {
    throw std::runtime_error("statistics database is unavailable");
  }
  return *backend_;
}

} // namespace inputcounter
