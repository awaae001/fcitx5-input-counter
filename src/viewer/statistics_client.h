// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_STATISTICS_CLIENT_H
#define FCITX5_INPUT_COUNTER_VIEWER_STATISTICS_CLIENT_H

//! Declares the asynchronous D-Bus client used by the viewer.

#include <cstdint>
#include <functional>
#include <variant>
#include <vector>

#include <QObject>
#include <QString>

#include "../statistics_types.h"
#include "statistics_snapshot.h"

namespace inputcounter {

/// Successful overview data or a human-readable transport error.
using SummaryResult = std::variant<StatisticsSummary, QString>;

/// Successful bucket values or a human-readable transport error.
using BucketResult = std::variant<std::vector<std::uint64_t>, QString>;

/// Successful completion or a human-readable transport error.
using ResetResult = std::variant<std::monostate, QString>;

/// Calls the input-counter interface without blocking the GUI thread.
class StatisticsClient final : public QObject {
public:
  /// Creates a client using the current session bus.
  explicit StatisticsClient(QObject *parent = nullptr);

  /// Requests overview values for the supplied local-time boundaries.
  void getSummary(const SummaryQuery &summary,
                  std::function<void(SummaryResult)> callback);

  /// Requests one count for each ordered, non-overlapping range.
  void getBucketCounts(const std::vector<TimeRange> &ranges,
                       std::function<void(BucketResult)> callback);

  /// Requests deletion of all persisted and pending statistics.
  void reset(std::function<void(ResetResult)> callback);
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_STATISTICS_CLIENT_H
