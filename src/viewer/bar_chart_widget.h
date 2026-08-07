// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_BAR_CHART_WIDGET_H
#define FCITX5_INPUT_COUNTER_VIEWER_BAR_CHART_WIDGET_H

//! Minimal bar chart widget used by the statistics viewer.

#include <cstdint>
#include <utility>
#include <vector>

#include <QSize>
#include <QString>
#include <QWidget>

namespace inputcounter {

/// Paints labeled vertical bars for hourly or daily totals.
class BarChartWidget final : public QWidget {
  Q_OBJECT

public:
  explicit BarChartWidget(QWidget *parent = nullptr);

  /// Replaces the displayed (label, value) bars and schedules a repaint.
  void setData(std::vector<std::pair<QString, std::uint64_t>> data);

  QSize minimumSizeHint() const override { return {360, 200}; }

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  std::vector<std::pair<QString, std::uint64_t>> data_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_BAR_CHART_WIDGET_H
