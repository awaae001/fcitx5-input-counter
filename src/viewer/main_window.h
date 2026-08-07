// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_H
#define FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_H

//! Declares the statistics viewer window.

#include <memory>

#include <QMainWindow>

class QLabel;

namespace inputcounter {

class BarChartWidget;
class StatsDb;

/// Shows totals and trend charts backed by the statistics database.
class MainWindow final : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

private:
  void refresh();
  void confirmReset();

  std::unique_ptr<StatsDb> db_;
  QLabel *totalValue_ = nullptr;
  QLabel *todayValue_ = nullptr;
  QLabel *last24HoursValue_ = nullptr;
  QLabel *last7DaysValue_ = nullptr;
  BarChartWidget *hoursChart_ = nullptr;
  BarChartWidget *weekChart_ = nullptr;
  BarChartWidget *monthChart_ = nullptr;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_H
