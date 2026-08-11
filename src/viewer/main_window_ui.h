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

  struct MainWindowUi final
  {
    QLabel &totalValue;
    QLabel &todayValue;
    QLabel &last24HoursValue;
    QLabel &last7DaysValue;
    QLabel &unavailableLabel;
    QLabel &connectionLabel;
    BarChartWidget &hoursChart;
    BarChartWidget &weekChart;
    BarChartWidget &monthChart;
    BarChartWidget &lastYearChart;
    BarChartWidget &allTimeChart;
    BarChartWidget &customChart;
    QTabBar &chartTabs;
    QStackedWidget &chartStack;
    QPushButton &customButton;
    QPushButton &refreshButton;
    QPushButton &clearButton;
    QShortcut &refreshShortcut;
  };

  /// Builds the viewer layout in window.
  MainWindowUi buildUi(QMainWindow &window);

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_UI_H
