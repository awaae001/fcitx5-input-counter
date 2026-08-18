// SPDX-License-Identifier: MIT

//! Implements the main-window layout.

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
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTabBar>
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
      QWidget &widget;
      BarChartWidget &hours;
      BarChartWidget &week;
      BarChartWidget &month;
      BarChartWidget &lastYear;
      BarChartWidget &allTime;
      BarChartWidget &custom;
      QTabBar &tabs;
      QStackedWidget &stack;
      QPushButton &customButton;
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
      auto *widget = new QWidget(parent);
      auto *layout = new QVBoxLayout(widget);
      layout->setContentsMargins(0, 0, 0, 0);
      layout->setSpacing(0);

      auto *header = new QHBoxLayout;
      header->setContentsMargins(0, 0, 0, 0);
      auto *tabs = new QTabBar(widget);
      tabs->setDocumentMode(true);
      tabs->setDrawBase(false);
      tabs->setExpanding(false);
      tabs->addTab(IC_("Last 24 hours"));
      tabs->addTab(IC_("Last 7 days"));
      tabs->addTab(IC_("Last 30 days"));
      tabs->addTab(IC_("Last 12 months"));
      tabs->addTab(IC_("All time"));

      auto *customButton = new QPushButton(IC_("Custom"), widget);
      customButton->setFlat(true);
      customButton->setCheckable(true);
      header->addWidget(tabs);
      header->addStretch(1);
      header->addWidget(customButton);
      layout->addLayout(header);

      auto *stack = new QStackedWidget(widget);
      layout->addWidget(stack, 1);

      auto *hours = new BarChartWidget(stack);
      auto *week = new BarChartWidget(stack);
      auto *month = new BarChartWidget(stack);
      auto *lastYear = new BarChartWidget(stack);
      auto *allTime = new BarChartWidget(stack);
      auto *custom = new BarChartWidget(stack);
      for (auto *chart : {hours, week, month, lastYear, allTime, custom})
      {
        stack->addWidget(chart);
      }

      QObject::connect(tabs, &QTabBar::currentChanged, stack,
                       [stack, customButton](int index)
                       {
                         if (index >= 0)
                         {
                           stack->setCurrentIndex(index);
                           customButton->setChecked(false);
                         }
                       });
      tabs->setCurrentIndex(0);
      return {*widget, *hours, *week, *month, *lastYear, *allTime,
              *custom, *tabs, *stack, *customButton};
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
        window.style()->standardIcon(QStyle::SP_TrashIcon), IC_("Clear all"),
        central);

    auto *unavailableLabel = new QLabel(IC_("Data unavailable"), central);
    unavailableLabel->setForegroundRole(QPalette::PlaceholderText);
    unavailableLabel->setTextFormat(Qt::PlainText);
    unavailableLabel->setWordWrap(true);

    auto *connectionLabel = new QLabel(&window);
    window.statusBar()->addPermanentWidget(connectionLabel);

    auto *sidePanel = new QVBoxLayout;
    sidePanel->setSpacing(10);
    sidePanel->addWidget(&overview.widget);
    sidePanel->addWidget(unavailableLabel);
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
    layout->addWidget(&charts.widget, 1);

    window.setCentralWidget(central);
    window.setMinimumSize(640, 360);
    window.resize(900, 400);

    return {overview.total,
            overview.today,
            overview.last24Hours,
            overview.last7Days,
            *unavailableLabel,
            *connectionLabel,
            charts.hours,
            charts.week,
            charts.month,
            charts.lastYear,
            charts.allTime,
            charts.custom,
            charts.tabs,
            charts.stack,
            charts.customButton,
            *refreshButton,
            *clearButton,
            *refreshShortcut};
  }

} // namespace inputcounter
