// SPDX-License-Identifier: MIT

//! Coordinates the statistics viewer window.

#include "main_window.h"

#include <exception>
#include <memory>
#include <utility>
#include <variant>

#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QString>
#include <QTabBar>
#include <QTimer>

#include "bar_chart_widget.h"
#include "chart_range.h"
#include "custom_range_dialog.h"
#include "i18n.h"
#include "main_window_ui.h"
#include "statistics_client.h"
#include "statistics_snapshot.h"

namespace inputcounter {

namespace {

constexpr int kRefreshIntervalMilliseconds = 60 * 1000;

QString connectionText(const char *color, const char *text) {
  return QStringLiteral("<span style=\"color:%1\">●</span> %2")
      .arg(QString::fromLatin1(color), QString(text).toHtmlEscaped());
}

struct SelectedChart final {
  BarChartWidget *widget;
  ChartQuery query;
};

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), client_(std::make_unique<StatisticsClient>()) {
  setWindowTitle(IC_("Input Counter"));

  ui_ = std::make_unique<MainWindowUi>(buildUi(*this));
  clearDisplay();
  setUnavailable();

  connect(&ui_->refreshButton, &QPushButton::clicked, this,
          &MainWindow::refresh);
  connect(&ui_->refreshShortcut, &QShortcut::activated, this,
          &MainWindow::refresh);
  connect(&ui_->clearButton, &QPushButton::clicked, this,
          &MainWindow::confirmReset);
  connect(&ui_->customButton, &QPushButton::clicked, this,
          &MainWindow::editCustomRange);
  connect(&ui_->chartTabs, &QTabBar::currentChanged, this,
          [this](int) { refresh(); });

  auto *timer = new QTimer(this);
  timer->setInterval(kRefreshIntervalMilliseconds);
  connect(timer, &QTimer::timeout, this, &MainWindow::refresh);
  timer->start();

  refresh();
}

MainWindow::~MainWindow() = default;

void MainWindow::refresh() {
  if (ui_ == nullptr) {
    return;
  }
  if (operationPending_) {
    refreshQueued_ = true;
    return;
  }

  operationPending_ = true;
  setBusy(true);
  const auto now = nowSeconds();
  client_->getSummary(summaryQuery(now), [this, now](SummaryResult result) {
    if (const auto *error = std::get_if<QString>(&result)) {
      Q_UNUSED(error);
      setUnavailable();
      finishOperation();
      return;
    }

    const auto summary = std::get<StatisticsSummary>(result);
    auto selected = [this, &summary, now]() -> SelectedChart {
      if (ui_->chartStack.currentWidget() == &ui_->customChart &&
          customRange_ != nullptr) {
        return {&ui_->customChart, customQuery(*customRange_)};
      }

      switch (ui_->chartTabs.currentIndex()) {
      case 1:
        return {&ui_->weekChart, last7DaysQuery(now)};
      case 2:
        return {&ui_->monthChart, last30DaysQuery(now)};
      case 3:
        return {&ui_->lastYearChart, last12MonthsQuery(now)};
      case 4:
        return {&ui_->allTimeChart,
                summary.hasData ? allTimeQuery(summary.firstHour, now)
                                : ChartQuery{}};
      case 0:
      default:
        return {&ui_->hoursChart, last24HoursQuery(now)};
      }
    }();
    client_->getBucketCounts(
        ranges(selected.query), [this, summary, selected = std::move(selected)](
                                    BucketResult bucketResult) mutable {
          if (const auto *error = std::get_if<QString>(&bucketResult)) {
            Q_UNUSED(error);
            setUnavailable();
            finishOperation();
            return;
          }

          try {
            selected.widget->setData(makeBars(
                selected.query,
                std::get<std::vector<std::uint64_t>>(std::move(bucketResult))));
          } catch (const std::exception &) {
            setUnavailable();
            finishOperation();
            return;
          }

          ui_->totalValue.setText(format(summary.total));
          ui_->todayValue.setText(format(summary.today));
          ui_->last24HoursValue.setText(format(summary.last24Hours));
          ui_->last7DaysValue.setText(format(summary.last7Days));
          available_ = true;
          ui_->unavailableLabel.hide();
          ui_->connectionLabel.setText(
              connectionText("#2e7d32", IC_("Connected")));
          hasSnapshot_ = true;
          finishOperation();
        });
  });
}

void MainWindow::editCustomRange() {
  if (ui_ == nullptr) {
    return;
  }

  const bool showingCustom =
      ui_->chartStack.currentWidget() == &ui_->customChart;
  auto selected = chooseCustomRange(*this, customRange_.get());
  if (!selected.has_value()) {
    ui_->customButton.setChecked(showingCustom);
    return;
  }

  customRange_ = std::make_unique<ChartRange>(std::move(selected).value());
  ui_->chartTabs.setCurrentIndex(-1);
  ui_->chartStack.setCurrentWidget(&ui_->customChart);
  ui_->customButton.setChecked(true);
  refresh();
}

void MainWindow::confirmReset() {
  if (ui_ == nullptr || operationPending_ || !available_) {
    return;
  }
  const auto choice = QMessageBox::question(
      this, IC_("Clear all statistics"),
      IC_("This permanently deletes all recorded input statistics. "
          "Continue?"));
  if (choice != QMessageBox::Yes) {
    return;
  }

  operationPending_ = true;
  setBusy(true);
  client_->reset([this](ResetResult result) {
    if (const auto *error = std::get_if<QString>(&result)) {
      QMessageBox::warning(
          this, IC_("Clear all statistics"),
          QString(IC_("Failed to clear statistics: %1")).arg(*error));
      setUnavailable();
      finishOperation();
      return;
    }

    clearDisplay();
    finishOperation();
    refresh();
  });
}

void MainWindow::setBusy(bool busy) {
  ui_->refreshButton.setEnabled(!busy);
  ui_->clearButton.setEnabled(!busy && available_);
}

void MainWindow::setUnavailable() {
  available_ = false;
  ui_->unavailableLabel.show();
  ui_->connectionLabel.setText(
      connectionText("#c62828", IC_("Data unavailable")));
  if (!hasSnapshot_) {
    clearDisplay();
  }
}

void MainWindow::finishOperation() {
  operationPending_ = false;
  setBusy(false);
  if (refreshQueued_) {
    refreshQueued_ = false;
    QTimer::singleShot(0, this, &MainWindow::refresh);
  }
}

void MainWindow::clearDisplay() {
  hasSnapshot_ = false;
  ui_->totalValue.setText(QStringLiteral("—"));
  ui_->todayValue.setText(QStringLiteral("—"));
  ui_->last24HoursValue.setText(QStringLiteral("—"));
  ui_->last7DaysValue.setText(QStringLiteral("—"));
  for (auto *chart :
       {&ui_->hoursChart, &ui_->weekChart, &ui_->monthChart,
        &ui_->lastYearChart, &ui_->allTimeChart, &ui_->customChart}) {
    chart->setData({});
  }
}

} // namespace inputcounter
