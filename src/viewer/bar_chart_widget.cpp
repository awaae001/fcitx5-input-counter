// SPDX-License-Identifier: MIT

//! Implements the bar chart widget.

#include "bar_chart_widget.h"

#include <algorithm>
#include <cmath>

#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

#include "i18n.h"

namespace inputcounter {

namespace {

/// Rounds value up to a "nice" 1/2/2.5/5 * 10^n ceiling for axis scaling.
double niceCeiling(double value) {
  if (value <= 0.0) {
    return 1.0;
  }
  const double magnitude = std::pow(10.0, std::floor(std::log10(value)));
  const double fraction = value / magnitude;
  double niceFraction = 10.0;
  if (fraction <= 1.0) {
    niceFraction = 1.0;
  } else if (fraction <= 2.0) {
    niceFraction = 2.0;
  } else if (fraction <= 2.5) {
    niceFraction = 2.5;
  } else if (fraction <= 5.0) {
    niceFraction = 5.0;
  }
  return niceFraction * magnitude;
}

QString formatAxisValue(double value) {
  return QLocale().toString(static_cast<qulonglong>(value));
}

} // namespace

BarChartWidget::BarChartWidget(QWidget *parent) : QWidget(parent) {
  setMouseTracking(true);
}

void BarChartWidget::setData(
    std::vector<std::pair<QString, std::uint64_t>> data) {
  data_ = std::move(data);
  update();
}

void BarChartWidget::paintEvent(QPaintEvent * /*event*/) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.fillRect(rect(), palette().color(QPalette::Base));

  const auto maxIt =
      std::max_element(data_.begin(), data_.end(), [](const auto &a,
                                                      const auto &b) {
        return a.second < b.second;
      });
  const std::uint64_t maxValue =
      maxIt == data_.end() ? 0 : maxIt->second;

  if (data_.empty() || maxValue == 0) {
    slotWidth_ = 0.0;
    painter.setPen(palette().color(QPalette::PlaceholderText));
    painter.drawText(rect(), Qt::AlignCenter, IC_("No data recorded yet"));
    return;
  }

  constexpr int kGridLines = 4;
  constexpr int kTopMargin = 10;
  constexpr int kBottomMargin = 26;
  constexpr int kRightMargin = 8;
  constexpr int kLabelGap = 6;

  const double axisMax = niceCeiling(static_cast<double>(maxValue));
  const QFontMetrics metrics = painter.fontMetrics();
  const int leftMargin =
      metrics.horizontalAdvance(formatAxisValue(axisMax)) + 2 * kLabelGap;

  const QRectF plot(leftMargin, kTopMargin,
                    width() - leftMargin - kRightMargin,
                    height() - kTopMargin - kBottomMargin);
  if (plot.width() <= 0 || plot.height() <= 0) {
    slotWidth_ = 0.0;
    return;
  }
  plotRect_ = plot;
  slotWidth_ = plot.width() / static_cast<int>(data_.size());

  // Horizontal grid lines with axis value labels.
  painter.setPen(palette().color(QPalette::Text));
  for (int i = 0; i <= kGridLines; ++i) {
    const double ratio = static_cast<double>(i) / kGridLines;
    const double y = plot.bottom() - ratio * plot.height();
    const double value = ratio * axisMax;

    painter.setPen(palette().color(QPalette::Mid));
    painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(
        QRectF(0, y - metrics.height() / 2.0, leftMargin - kLabelGap,
               metrics.height()),
        Qt::AlignRight | Qt::AlignVCenter, formatAxisValue(value));
  }

  // Baseline and left axis.
  painter.setPen(QPen(palette().color(QPalette::Dark), 1));
  painter.drawLine(QPointF(plot.left(), plot.bottom()),
                   QPointF(plot.right(), plot.bottom()));
  painter.drawLine(QPointF(plot.left(), plot.top()),
                   QPointF(plot.left(), plot.bottom()));

  const auto count = static_cast<int>(data_.size());
  const double slotWidth = slotWidth_;
  const auto barHeightFor = [axisMax, &plot](double value) {
    return value > 0.0
               ? std::max(2.0, value / axisMax * plot.height())
               : 0.0;
  };
  const double barWidth = std::max(2.0, slotWidth * 0.65);
  const auto maxLabels = std::max(1, static_cast<int>(plot.width()) / 36);
  const int labelEvery = std::max(1, (count + maxLabels - 1) / maxLabels);

  const QColor barColor = palette().color(QPalette::Highlight);
  const QColor hoverColor = barColor.lighter(125);
  for (int i = 0; i < count; ++i) {
    const double value = static_cast<double>(data_[i].second);
    const double x =
        plot.left() + i * slotWidth + (slotWidth - barWidth) / 2.0;

    if (value > 0) {
      const double barHeight = barHeightFor(value);
      const QRectF bar(x, plot.bottom() - barHeight, barWidth, barHeight);
      const QColor color = i == hoverIndex_ ? hoverColor : barColor;
      if (barHeight > 8.0) {
        QPainterPath path;
        path.addRoundedRect(bar, 3.0, 3.0);
        painter.fillPath(path, color);
      } else {
        painter.fillRect(bar, color);
      }
    }

    if (i % labelEvery == 0) {
      const double labelWidth = slotWidth * labelEvery;
      const QRectF labelRect(plot.left() + i * slotWidth -
                                 (labelWidth - slotWidth) / 2.0,
                             plot.bottom() + 4.0, labelWidth,
                             kBottomMargin - 6.0);
      painter.setPen(palette().color(QPalette::Text));
      painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop,
                       data_[i].first);
    }
  }

  // Exact value above the hovered bar.
  if (hoverIndex_ >= 0 && hoverIndex_ < count) {
    const double value = static_cast<double>(data_[hoverIndex_].second);
    const QString text = formatAxisValue(value);
    const double slotX = plot.left() + hoverIndex_ * slotWidth;
    const double barTop = plot.bottom() - barHeightFor(value);
    QFont font = painter.font();
    font.setBold(true);
    painter.setFont(font);
    const int textWidth =
        painter.fontMetrics().horizontalAdvance(text) + 4;
    const double textX = std::clamp(
        slotX + (slotWidth - textWidth) / 2.0, plot.left(),
        plot.right() - textWidth);
    const double textY =
        std::max(plot.top(), barTop - metrics.height() - 2.0);
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(QRectF(textX, textY, textWidth, metrics.height()),
                     Qt::AlignHCenter | Qt::AlignVCenter, text);
  }
}

void BarChartWidget::mouseMoveEvent(QMouseEvent *event) {
  int index = -1;
  if (!data_.empty() && slotWidth_ > 0.0 &&
      plotRect_.contains(event->position())) {
    index = static_cast<int>((event->position().x() - plotRect_.left()) /
                             slotWidth_);
    index = std::clamp(index, 0, static_cast<int>(data_.size()) - 1);
  }
  if (index != hoverIndex_) {
    hoverIndex_ = index;
    update();
  }
  if (index >= 0) {
    QToolTip::showText(
        event->globalPosition().toPoint(),
        QStringLiteral("%1: %2")
            .arg(data_[index].first,
                 QLocale().toString(
                     static_cast<qulonglong>(data_[index].second))),
        this);
  } else {
    QToolTip::hideText();
  }
  QWidget::mouseMoveEvent(event);
}

void BarChartWidget::leaveEvent(QEvent *event) {
  if (hoverIndex_ != -1) {
    hoverIndex_ = -1;
    update();
  }
  QToolTip::hideText();
  QWidget::leaveEvent(event);
}

} // namespace inputcounter
