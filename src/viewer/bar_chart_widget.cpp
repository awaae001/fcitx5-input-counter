// SPDX-License-Identifier: MIT

//! Owns bar-chart data and pointer interaction.

#include "bar_chart_widget.h"

#include <algorithm>

#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include "bar_chart_renderer.h"

namespace inputcounter
{

  BarChartWidget::BarChartWidget(QWidget *parent) : QWidget(parent)
  {
    setMouseTracking(true);
  }

  void BarChartWidget::setData(
      std::vector<std::pair<QString, std::uint64_t>> data)
  {
    data_ = std::move(data);
    update();
  }

  void BarChartWidget::paintEvent(QPaintEvent * /*event*/)
  {
    QPainter painter(this);
    const auto geometry = paint(painter, rect(), palette(), data_, hoverIndex_);
    plotRect_ = geometry.plot;
    slotWidth_ = geometry.slotWidth;
  }

  void BarChartWidget::mouseMoveEvent(QMouseEvent *event)
  {
    int index = -1;
    if (!data_.empty() && slotWidth_ > 0.0 &&
        plotRect_.contains(event->position()))
    {
      index = static_cast<int>((event->position().x() - plotRect_.left()) /
                               slotWidth_);
      index = std::clamp(index, 0, static_cast<int>(data_.size()) - 1);
    }

    if (index != hoverIndex_)
    {
      hoverIndex_ = index;
      update();
    }

    if (index >= 0)
    {
      QToolTip::showText(
          event->globalPosition().toPoint(),
          QStringLiteral("%1: %2")
              .arg(data_[index].first,
                   QLocale().toString(
                       static_cast<qulonglong>(data_[index].second))),
          this);
    }
    else
    {
      QToolTip::hideText();
    }
    QWidget::mouseMoveEvent(event);
  }

  void BarChartWidget::leaveEvent(QEvent *event)
  {
    if (hoverIndex_ != -1)
    {
      hoverIndex_ = -1;
      update();
    }
    QToolTip::hideText();
    QWidget::leaveEvent(event);
  }

} // namespace inputcounter
