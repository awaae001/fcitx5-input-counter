// SPDX-License-Identifier: MIT

//! Implements the statistics viewer window.

#include "main_window.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <exception>
#include <map>
#include <utility>
#include <vector>

#include <QDateTime>
#include <QKeySequence>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QStyle>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "../stats_db.h"
#include "bar_chart_widget.h"
#include "i18n.h"

namespace inputcounter
{

  namespace
  {

    std::int64_t nowSeconds()
    {
      return std::chrono::duration_cast<std::chrono::seconds>(
                 std::chrono::system_clock::now().time_since_epoch())
          .count();
    }

    /// Returns the Unix timestamp of the start of the local day containing ts.
    std::int64_t localDayStart(std::int64_t ts)
    {
      const auto time = static_cast<std::time_t>(ts);
      std::tm local{};
      localtime_r(&time, &local);
      local.tm_hour = 0;
      local.tm_min = 0;
      local.tm_sec = 0;
      return static_cast<std::int64_t>(std::mktime(&local));
    }

    QString formatCount(std::uint64_t value)
    {
      return QLocale().toString(static_cast<qulonglong>(value));
    }

    std::uint64_t lookup(const std::map<std::int64_t, std::uint64_t> &counts,
                         std::int64_t key)
    {
      const auto it = counts.find(key);
      return it == counts.end() ? 0 : it->second;
    }

    /// Creates a bold, right-aligned value label for the overview grid.
    QLabel *createValueLabel(QWidget *parent)
    {
      auto *label = new QLabel(parent);
      QFont font = label->font();
      font.setBold(true);
      label->setFont(font);
      font.setPointSizeF(font.pointSizeF() * 1.15);
      label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      return label;
    }

  } // namespace

  MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
  {
    setWindowTitle(IC_("Input statistics"));

    auto *central = new QWidget(this);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    QString databasePath;
    try
    {
      databasePath = QString::fromStdString(statsDatabasePath());
      db_ = std::make_unique<StatsDb>(databasePath.toStdString());
    }
    catch (const std::exception &error)
    {
      auto *errorLabel = new QLabel(
          QString(IC_("Could not open the statistics database: %1"))
              .arg(error.what()),
          central);
      errorLabel->setWordWrap(true);
      layout->addWidget(errorLabel);
      setCentralWidget(central);
      return;
    }

    auto *overview = new QWidget(central);
    auto *overviewLayout = new QVBoxLayout(overview);
    overviewLayout->setContentsMargins(0, 0, 0, 0);
    overviewLayout->setSpacing(2);
    const struct
    {
      const char *name;
      QLabel **value;
    } rows[] = {
        {IC_("Total"), &totalValue_},
        {IC_("Today"), &todayValue_},
        {IC_("Last 24 hours"), &last24HoursValue_},
        {IC_("Last 7 days"), &last7DaysValue_},
    };
    // Two rows per entry (name over value) so large counts always get the
    // full sidebar width and can never collide with the name.
    for (int i = 0; i < 4; ++i)
    {
      if (i > 0)
      {
        overviewLayout->addSpacing(10);
      }
      auto *name = new QLabel(rows[i].name, overview);
      name->setForegroundRole(QPalette::PlaceholderText);
      overviewLayout->addWidget(name);
      *rows[i].value = createValueLabel(overview);
      overviewLayout->addWidget(*rows[i].value);
    }

    auto *tabs = new QTabWidget(central);
    tabs->setDocumentMode(true);
    hoursChart_ = new BarChartWidget(tabs);
    weekChart_ = new BarChartWidget(tabs);
    monthChart_ = new BarChartWidget(tabs);
    tabs->addTab(hoursChart_, IC_("Last 24 hours"));
    tabs->addTab(weekChart_, IC_("Last 7 days"));
    tabs->addTab(monthChart_, IC_("Last 30 days"));

    auto *refreshButton = new QPushButton(
        style()->standardIcon(QStyle::SP_BrowserReload), IC_("Refresh"),
        central);
    // Keep the button label clean; F5 lives in the tooltip and a shortcut.
    const QKeySequence refreshKeys(QKeySequence::Refresh);
    refreshButton->setToolTip(refreshKeys.toString());
    auto *refreshShortcut = new QShortcut(refreshKeys, this);
    connect(refreshShortcut, &QShortcut::activated, this,
            [this]
            { refresh(); });
    auto *clearButton = new QPushButton(
        style()->standardIcon(QStyle::SP_TrashIcon), IC_("Clear all…"),
        central);

    // Sidebar: overview on top, actions pinned to the bottom.
    auto *sidePanel = new QVBoxLayout;
    sidePanel->setSpacing(10);
    sidePanel->addWidget(overview);
    sidePanel->addStretch(1);
    sidePanel->addWidget(refreshButton);
    sidePanel->addWidget(clearButton);

    // Labels happily shrink below their text width, so pin the sidebar to
    // the widest content instead of letting the tab widget squeeze it.
    auto *sideWidget = new QWidget(central);
    sideWidget->setLayout(sidePanel);
    // Double width: the stats values are the focus of the sidebar.
    sideWidget->setMinimumWidth(2 * std::max({overview->sizeHint().width(),
                                              refreshButton->sizeHint().width(),
                                              clearButton->sizeHint().width()}));
    layout->addWidget(sideWidget, 0);
    layout->addWidget(tabs, 1);

    setCentralWidget(central);
    setMinimumSize(640, 360);
    resize(900, 400);

    connect(refreshButton, &QPushButton::clicked, this, [this]
            { refresh(); });
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::confirmReset);

    auto *timer = new QTimer(this);
    timer->setInterval(5000);
    connect(timer, &QTimer::timeout, this, [this]
            { refresh(); });
    timer->start();

    refresh();
  }

  MainWindow::~MainWindow() = default;

  void MainWindow::refresh()
  {
    if (db_ == nullptr)
    {
      return;
    }

    try
    {
      const std::int64_t now = nowSeconds();
      const std::int64_t todayStart = localDayStart(now);

      totalValue_->setText(formatCount(db_->totalChars()));
      todayValue_->setText(formatCount(db_->charsSince(todayStart)));
      last24HoursValue_->setText(formatCount(db_->charsSince(now - 24 * 3600)));
      last7DaysValue_->setText(
          formatCount(db_->charsSince(todayStart - 6 * 24 * 3600)));

      std::map<std::int64_t, std::uint64_t> byHour;
      for (const auto &row : db_->hourlySince(now - 30 * 24 * 3600))
      {
        byHour[row.hour] += row.chars;
      }

      std::vector<std::pair<QString, std::uint64_t>> bars;
      for (int i = 23; i >= 0; --i)
      {
        const std::int64_t hour = hourStartOf(now - i * 3600);
        bars.emplace_back(
            QDateTime::fromSecsSinceEpoch(hour).toString(QStringLiteral("HH")),
            lookup(byHour, hour));
      }
      hoursChart_->setData(std::move(bars));

      std::map<std::int64_t, std::uint64_t> byDay;
      for (const auto &[hour, chars] : byHour)
      {
        byDay[localDayStart(hour)] += chars;
      }
      for (const auto &[chart, days] :
           {std::pair<BarChartWidget *, int>{weekChart_, 7},
            {monthChart_, 30}})
      {
        std::vector<std::pair<QString, std::uint64_t>> dayBars;
        for (int i = days - 1; i >= 0; --i)
        {
          const std::int64_t day = localDayStart(now - i * 24 * 3600);
          dayBars.emplace_back(
              QDateTime::fromSecsSinceEpoch(day).toString(
                  QStringLiteral("MM-dd")),
              lookup(byDay, day));
        }
        chart->setData(std::move(dayBars));
      }
    }
    catch (const std::exception &error)
    {
      QMessageBox::warning(this, IC_("Input statistics"),
                           QString(IC_("Failed to read statistics: %1"))
                               .arg(error.what()));
    }
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
