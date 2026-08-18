// SPDX-License-Identifier: MIT

//! Implements the addon's versioned D-Bus statistics interface.

#include "input_counter_dbus.h"

#include <exception>
#include <stdexcept>

#include <fcitx-utils/dbus/objectvtable.h>

#include "statistics_backend.h"

namespace inputcounter
{

  namespace
  {
    constexpr char kUnavailableError[] =
        "org.fcitx.Fcitx.InputCounter1.Error.DataUnavailable";
    constexpr char kInvalidArgumentsError[] =
        "org.freedesktop.DBus.Error.InvalidArgs";

    template <typename Function>
    auto translateErrors(Function &&function)
    {
      try
      {
        return function();
      }
      catch (const std::invalid_argument &error)
      {
        throw fcitx::dbus::MethodCallError(kInvalidArgumentsError, error.what());
      }
      catch (const std::exception &error)
      {
        throw fcitx::dbus::MethodCallError(kUnavailableError, error.what());
      }
    }

  } // namespace

  InputCounterDBus::InputCounterDBus(StatisticsBackend &backend) noexcept
      : backend_(backend) {}

  InputCounterDBus::Summary
  InputCounterDBus::getSummary(std::int64_t todayStart,
                               std::int64_t last24HoursStart,
                               std::int64_t last7DaysStart)
  {
    return translateErrors([this, todayStart, last24HoursStart, last7DaysStart]
                           {
    const auto result =
        backend_.summary(todayStart, last24HoursStart, last7DaysStart);
    return Summary{result.total,     result.today,   result.last24Hours,
                   result.last7Days, result.hasData, result.firstHour}; });
  }

  std::vector<std::uint64_t>
  InputCounterDBus::getBucketCounts(const std::vector<Bucket> &buckets)
  {
    return translateErrors([this, &buckets]
                           {
    std::vector<TimeRange> ranges;
    ranges.reserve(buckets.size());
    for (const auto &bucket : buckets) {
      const auto &[start, end] = bucket.data();
      ranges.push_back({start, end});
    }
    return backend_.bucketCounts(ranges); });
  }

  void InputCounterDBus::reset()
  {
    translateErrors([this]
                    { backend_.reset(); });
  }

} // namespace inputcounter
