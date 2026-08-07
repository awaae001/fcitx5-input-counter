// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_UI_H
#define FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_UI_H

//! Builds and names the widgets owned by the main window.

class QLabel;
class QMainWindow;
class QPushButton;
class QShortcut;

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
    /// Manual refresh button.
    QPushButton &refreshButton;
    /// Clear-data button.
    QPushButton &clearButton;
    /// Refresh keyboard shortcut.
    QShortcut &refreshShortcut;
  };

  /// Builds the unchanged viewer layout in window.
  MainWindowUi buildUi(QMainWindow &window);

  /// Replaces window's contents with the database-open error message.
  void showDbError(QMainWindow &window, const char *message);

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_UI_H
