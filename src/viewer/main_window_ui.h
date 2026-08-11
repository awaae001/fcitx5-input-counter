// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_UI_H
#define FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_UI_H

//! Builds and names the widgets owned by the main window.

class QLabel;
class QMainWindow;
class QPushButton;
class QShortcut;
class QStackedWidget;
class QTabBar;

namespace inputcounter
{

  class BarChartWidget;

  /// Borrows the widgets owned by a fully built main window.
  struct MainWindowUi final
  {
    /// Total count label.
    QLabel &totalValue;
    /// Today's count label.
    QLabel &todayValue;
    /// Rolling 24-hour count label.
    QLabel &last24HoursValue;
    /// Seven-day count label.
    QLabel &last7DaysValue;
    /// Reports that the addon cannot currently provide statistics.
    QLabel &unavailableLabel;
    /// Shows the current addon connection state in the window corner.
    QLabel &connectionLabel;
    /// Hourly chart.
    BarChartWidget &hoursChart;
    /// Seven-day chart.
    BarChartWidget &weekChart;
    /// Thirty-day chart.
    BarChartWidget &monthChart;
    /// Twelve-month chart.
    BarChartWidget &lastYearChart;
    /// All-time chart.
    BarChartWidget &allTimeChart;
    /// Custom-range chart.
    BarChartWidget &customChart;
    /// Preset chart tabs shown at the left of the chart header.
    QTabBar &chartTabs;
    /// Stack containing preset and custom charts.
    QStackedWidget &chartStack;
    /// Opens the custom-range modal dialog.
    QPushButton &customButton;
    /// Manual refresh button.
    QPushButton &refreshButton;
    /// Clear-data button.
    QPushButton &clearButton;
    /// Refresh keyboard shortcut.
    QShortcut &refreshShortcut;
  };

  /// Builds the viewer layout in window.
  MainWindowUi buildUi(QMainWindow &window);

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_UI_H
