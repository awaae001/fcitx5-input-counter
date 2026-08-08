// SPDX-License-Identifier: MIT

//! Coordinates the statistics viewer window.

#include "main_window.h"

#include <exception>
#include <memory>
#include <utility>

#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QString>
#include <QTabBar>
#include <QTimer>

#include "../stats_db.h"
#include "bar_chart_widget.h"
#include "chart_range.h"
#include "custom_range_dialog.h"
#include "i18n.h"
#include "main_window_ui.h"
#include "statistics_snapshot.h"

namespace inputcounter
{

  MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
  {
    setWindowTitle(IC_("Input Counter"));

    try
    {
      const auto path = QString::fromStdString(statsDatabasePath());
      db_ = std::make_unique<StatsDb>(path.toStdString());
    }
    catch (const std::exception &error)
    {
      showDbError(*this, error.what());
      return;
    }

    ui_ = std::make_unique<MainWindowUi>(buildUi(*this));

    connect(&ui_->refreshButton, &QPushButton::clicked, this,
            &MainWindow::refresh);
    connect(&ui_->refreshShortcut, &QShortcut::activated, this,
            &MainWindow::refresh);
    connect(&ui_->clearButton, &QPushButton::clicked, this,
            &MainWindow::confirmReset);
    connect(&ui_->customButton, &QPushButton::clicked, this,
            &MainWindow::editCustomRange);

    auto *timer = new QTimer(this);
    timer->setInterval(5000);
    connect(timer, &QTimer::timeout, this, &MainWindow::refresh);
    timer->start();

    refresh();
  }

  MainWindow::~MainWindow() = default;

  void MainWindow::refresh()
  {
    if (db_ == nullptr || ui_ == nullptr)
    {
      return;
    }

    try
    {
      auto data = load(*db_, nowSeconds());
      ui_->totalValue.setText(format(data.total));
      ui_->todayValue.setText(format(data.today));
      ui_->last24HoursValue.setText(format(data.last24Hours));
      ui_->last7DaysValue.setText(format(data.last7Days));
      ui_->hoursChart.setData(std::move(data.hours));
      ui_->weekChart.setData(std::move(data.week));
      ui_->monthChart.setData(std::move(data.month));
      ui_->lastYearChart.setData(std::move(data.lastYear));
      ui_->allTimeChart.setData(std::move(data.allTime));
      if (customRange_ != nullptr)
      {
        ui_->customChart.setData(load(*db_, *customRange_));
      }
    }
    catch (const std::exception &error)
    {
      QMessageBox::warning(this, IC_("Input Counter"),
                           QString(IC_("Failed to read statistics: %1"))
                               .arg(error.what()));
    }
  }

  void MainWindow::editCustomRange()
  {
    if (db_ == nullptr || ui_ == nullptr)
    {
      return;
    }

    const bool showingCustom =
        ui_->chartStack.currentWidget() == &ui_->customChart;
    auto selected = chooseCustomRange(*this, customRange_.get());
    if (!selected.has_value())
    {
      ui_->customButton.setChecked(showingCustom);
      return;
    }

    customRange_ =
        std::make_unique<ChartRange>(std::move(selected).value());
    ui_->chartTabs.setCurrentIndex(-1);
    ui_->chartStack.setCurrentWidget(&ui_->customChart);
    ui_->customButton.setChecked(true);
    refresh();
  }

  void MainWindow::confirmReset()
  {
    const auto choice = QMessageBox::question(
        this, IC_("Clear all statistics"),
        IC_("This permanently deletes all recorded input statistics. "
            "Continue?"));
    if (choice != QMessageBox::Yes)
    {
      return;
    }

    try
    {
      db_->reset();
    }
    catch (const std::exception &error)
    {
      QMessageBox::warning(this, IC_("Clear all statistics"),
                           QString(IC_("Failed to clear statistics: %1"))
                               .arg(error.what()));
    }
    refresh();
  }

} // namespace inputcounter
