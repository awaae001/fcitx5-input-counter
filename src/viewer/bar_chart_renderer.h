// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_BAR_CHART_RENDERER_H
#define FCITX5_INPUT_COUNTER_VIEWER_BAR_CHART_RENDERER_H

//! Paints bar-chart data without owning widget state.

#include <cstdint>
#include <utility>
#include <vector>

#include <QRect>
#include <QRectF>
#include <QString>

class QPainter;
class QPalette;

namespace inputcounter {

/// Geometry retained by the widget for pointer hit testing.
struct BarChartGeometry final {
  /// Last valid plot rectangle.
  QRectF plot;
  /// Width allocated to one bar and label.
  double slotWidth = 0.0;
};

/// Paints data into bounds and returns its hit-test geometry.
BarChartGeometry paint(QPainter &painter, const QRect &bounds,
                       const QPalette &palette,
                       const std::vector<std::pair<QString, std::uint64_t>> &data,
                       int hoverIndex);

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_BAR_CHART_RENDERER_H
