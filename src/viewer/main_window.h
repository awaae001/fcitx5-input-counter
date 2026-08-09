// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_H
#define FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_H

//! Declares the statistics viewer window.

#include <memory>

#include <QMainWindow>

namespace inputcounter
{

  class MainWindowUi;
  class ChartRange;
  class DatabaseManager;

  /// Shows totals and trend charts backed by the statistics database.
  class MainWindow final : public QMainWindow
  {
    Q_OBJECT

  public:
    /// Creates a viewer that borrows database for its entire lifetime.
    explicit MainWindow(DatabaseManager &database, QWidget *parent = nullptr);
    ~MainWindow() override;

  private:
    void refresh();
    void editCustomRange();
    void confirmReset();

    DatabaseManager &database_;
    std::unique_ptr<MainWindowUi> ui_;
    std::unique_ptr<ChartRange> customRange_;
  };

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_MAIN_WINDOW_H
