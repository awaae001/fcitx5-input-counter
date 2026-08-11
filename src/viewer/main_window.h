// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_H
#define FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_H

//! Declares the statistics viewer window.

#include <memory>

#include <QMainWindow>

namespace inputcounter {

class ChartRange;
class MainWindowUi;
class StatisticsClient;

/// Shows statistics obtained from the running Fcitx addon.
class MainWindow final : public QMainWindow {
  Q_OBJECT

public:
  /// Creates a viewer connected to the addon's session-bus interface.
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override;

private:
  void refresh();
  void editCustomRange();
  void confirmReset();
  void setBusy(bool busy);
  void setConnected();
  void setUnavailable();
  void finishOperation();
  void clearDisplay();

  std::unique_ptr<StatisticsClient> client_;
  std::unique_ptr<MainWindowUi> ui_;
  std::unique_ptr<ChartRange> customRange_;
  bool operationPending_ = false;
  bool refreshQueued_ = false;
  bool available_ = false;
  bool hasSnapshot_ = false;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_H
