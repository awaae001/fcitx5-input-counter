// SPDX-License-Identifier: MIT

//! Implements the fixed main-window layout.

#include "main_window_ui.h"

#include <algorithm>
#include <array>
#include <utility>

#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QShortcut>
#include <QStyle>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "bar_chart_widget.h"
#include "i18n.h"

namespace inputcounter
{

  namespace
  {

    struct Overview final
    {
      QWidget &widget;
      QLabel &total;
      QLabel &today;
      QLabel &last24Hours;
      QLabel &last7Days;
    };

    struct Charts final
    {
      QTabWidget &tabs;
      BarChartWidget &hours;
      BarChartWidget &week;
      BarChartWidget &month;
    };

    QLabel *valueLabel(QWidget *parent)
    {
      auto *label = new QLabel(parent);
      QFont font = label->font();
      font.setBold(true);
      label->setFont(font);
      font.setPointSizeF(font.pointSizeF() * 1.15);
      label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      return label;
    }

    Overview buildOverview(QWidget *parent)
    {
      auto *overview = new QWidget(parent);
      auto *layout = new QVBoxLayout(overview);
      layout->setContentsMargins(0, 0, 0, 0);
      layout->setSpacing(2);

      QLabel *total = nullptr;
      QLabel *today = nullptr;
      QLabel *last24Hours = nullptr;
      QLabel *last7Days = nullptr;
      const std::array rows{
          std::pair{IC_("Total"), &total},
          std::pair{IC_("Today"), &today},
          std::pair{IC_("Last 24 hours"), &last24Hours},
          std::pair{IC_("Last 7 days"), &last7Days},
      };

      bool first = true;
      for (const auto &[name, value] : rows)
      {
        if (!first)
        {
          layout->addSpacing(10);
        }
        first = false;

        auto *nameLabel = new QLabel(name, overview);
        nameLabel->setForegroundRole(QPalette::PlaceholderText);
        layout->addWidget(nameLabel);

        *value = valueLabel(overview);
        layout->addWidget(*value);
      }
      return {*overview, *total, *today, *last24Hours, *last7Days};
    }

    Charts buildCharts(QWidget *parent)
    {
      auto *tabs = new QTabWidget(parent);
      tabs->setDocumentMode(true);

      auto *hours = new BarChartWidget(tabs);
      auto *week = new BarChartWidget(tabs);
      auto *month = new BarChartWidget(tabs);
      tabs->addTab(hours, IC_("Last 24 hours"));
      tabs->addTab(week, IC_("Last 7 days"));
      tabs->addTab(month, IC_("Last 30 days"));
      return {*tabs, *hours, *week, *month};
    }

  } // namespace

  MainWindowUi buildUi(QMainWindow &window)
  {
    auto *central = new QWidget(&window);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto overview = buildOverview(central);
    auto charts = buildCharts(central);

    auto *refreshButton = new QPushButton(
        window.style()->standardIcon(QStyle::SP_BrowserReload), IC_("Refresh"),
        central);
    const QKeySequence refreshKeys(QKeySequence::Refresh);
    refreshButton->setToolTip(refreshKeys.toString());
    auto *refreshShortcut = new QShortcut(refreshKeys, &window);

    auto *clearButton = new QPushButton(
        window.style()->standardIcon(QStyle::SP_TrashIcon), IC_("Clear all…"),
        central);

    auto *sidePanel = new QVBoxLayout;
    sidePanel->setSpacing(10);
    sidePanel->addWidget(&overview.widget);
    sidePanel->addStretch(1);
    sidePanel->addWidget(refreshButton);
    sidePanel->addWidget(clearButton);

    auto *sideWidget = new QWidget(central);
    sideWidget->setLayout(sidePanel);
    sideWidget->setMinimumWidth(
        2 * std::max({overview.widget.sizeHint().width(),
                      refreshButton->sizeHint().width(),
                      clearButton->sizeHint().width()}));
    layout->addWidget(sideWidget, 0);
    layout->addWidget(&charts.tabs, 1);

    window.setCentralWidget(central);
    window.setMinimumSize(640, 360);
    window.resize(900, 400);

    return {overview.total,
            overview.today,
            overview.last24Hours,
            overview.last7Days,
            charts.hours,
            charts.week,
            charts.month,
            *refreshButton,
            *clearButton,
            *refreshShortcut};
  }

  void showDbError(QMainWindow &window, const char *message)
  {
    auto *central = new QWidget(&window);
    auto *layout = new QHBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto *label = new QLabel(
        QString(IC_("Could not open the statistics database: %1")).arg(message),
        central);
    label->setWordWrap(true);
    layout->addWidget(label);
    window.setCentralWidget(central);
  }

} // namespace inputcounter
