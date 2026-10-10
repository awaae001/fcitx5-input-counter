// SPDX-License-Identifier: MIT

//! Implements bar-chart painting, data, and pointer interaction.

#include "bar_chart_widget.h"

#include <algorithm>
#include <cmath>

#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QToolTip>

#include <fcitx-utils/i18n.h>

namespace inputcounter
{
  namespace
  {
    using Bars = std::vector<std::pair<QString, std::uint64_t>>;

    constexpr int kGridLines = 4;
    constexpr int kTopMargin = 10;
    constexpr int kBottomMargin = 26;
    constexpr int kRightMargin = 8;
    constexpr int kLabelGap = 6;

    QString format(double value)
    {
      return QLocale().toString(static_cast<qulonglong>(value));
    }

    double barHeight(double value, double axisMax, double plotHeight)
    {
      return value > 0.0 ? std::max(2.0, value / axisMax * plotHeight) : 0.0;
    }

    void paintGrid(QPainter &painter, const QRectF &plot, int leftMargin,
                   double axisMax, const QFontMetrics &metrics,
                   const QPalette &palette)
    {
      for (int index = 0; index <= kGridLines; ++index)
      {
        const double ratio = static_cast<double>(index) / kGridLines;
        const double y = plot.bottom() - ratio * plot.height();

        painter.setPen(palette.color(QPalette::Mid));
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));

        painter.setPen(palette.color(QPalette::Text));
        painter.drawText(
            QRectF(0, y - metrics.height() / 2.0, leftMargin - kLabelGap,
                   metrics.height()),
            Qt::AlignRight | Qt::AlignVCenter, format(ratio * axisMax));
      }
    }

    void paintBars(QPainter &painter, const Bars &data, const QRectF &plot,
                   double axisMax, double slotWidth, int hoverIndex,
                   const QPalette &palette)
    {
      const auto count = static_cast<int>(data.size());
      const double width = std::max(2.0, slotWidth * 0.65);
      const int maxLabels = std::max(1, static_cast<int>(plot.width()) / 36);
      const int labelEvery = std::max(1, (count + maxLabels - 1) / maxLabels);
      const QColor barColor = palette.color(QPalette::Highlight);
      const QColor hoverColor = barColor.lighter(125);

      for (int index = 0; index < count; ++index)
      {
        const double value = static_cast<double>(data[index].second);
        const double x =
            plot.left() + index * slotWidth + (slotWidth - width) / 2.0;

        if (value > 0.0)
        {
          const double height = barHeight(value, axisMax, plot.height());
          const QRectF bar(x, plot.bottom() - height, width, height);
          const QColor color = index == hoverIndex ? hoverColor : barColor;
          if (height > 8.0)
          {
            QPainterPath path;
            path.addRoundedRect(bar, 3.0, 3.0);
            painter.fillPath(path, color);
          }
          else
          {
            painter.fillRect(bar, color);
          }
        }

        if (index % labelEvery == 0)
        {
          const double labelWidth = slotWidth * labelEvery;
          const QRectF label(
              plot.left() + index * slotWidth -
                  (labelWidth - slotWidth) / 2.0,
              plot.bottom() + 4.0, labelWidth, kBottomMargin - 6.0);
          painter.setPen(palette.color(QPalette::Text));
          painter.drawText(label, Qt::AlignHCenter | Qt::AlignTop,
                           data[index].first);
        }
      }
    }

    void paintHover(QPainter &painter, const Bars &data, const QRectF &plot,
                    double axisMax, double slotWidth, int hoverIndex,
                    const QFontMetrics &metrics, const QPalette &palette)
    {
      if (hoverIndex < 0 || hoverIndex >= static_cast<int>(data.size()))
      {
        return;
      }

      const double value = static_cast<double>(data[hoverIndex].second);
      const QString text = format(value);
      const double slotX = plot.left() + hoverIndex * slotWidth;
      const double barTop =
          plot.bottom() - barHeight(value, axisMax, plot.height());

      QFont font = painter.font();
      font.setBold(true);
      painter.setFont(font);
      const int textWidth = painter.fontMetrics().horizontalAdvance(text) + 4;
      const double maximumTextX =
          std::max(plot.left(), plot.right() - textWidth);
      const double textX =
          std::clamp(slotX + (slotWidth - textWidth) / 2.0, plot.left(),
                     maximumTextX);
      const double textY =
          std::max(plot.top(), barTop - metrics.height() - 2.0);

      painter.setPen(palette.color(QPalette::Text));
      painter.drawText(QRectF(textX, textY, textWidth, metrics.height()),
                       Qt::AlignHCenter | Qt::AlignVCenter, text);
    }

  } // namespace

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
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), palette().color(QPalette::Base));

    const auto maximumEntry = std::max_element(
        data_.begin(), data_.end(), [](const auto &left, const auto &right)
        { return left.second < right.second; });
    const auto maximum =
        maximumEntry == data_.end() ? 0 : maximumEntry->second;
    if (data_.empty() || maximum == 0)
    {
      plotRect_ = {};
      slotWidth_ = 0.0;
      painter.setPen(palette().color(QPalette::PlaceholderText));
      painter.drawText(rect(), Qt::AlignCenter, _("No data recorded yet"));
      return;
    }

    const auto value = static_cast<double>(maximum);
    const double magnitude = std::pow(10.0, std::floor(std::log10(value)));
    const double fraction = value / magnitude;
    double roundedFraction = 10.0;
    if (fraction <= 1.0)
      roundedFraction = 1.0;
    else if (fraction <= 2.0)
      roundedFraction = 2.0;
    else if (fraction <= 2.5)
      roundedFraction = 2.5;
    else if (fraction <= 5.0)
      roundedFraction = 5.0;

    const double axisMax = roundedFraction * magnitude;
    const QFontMetrics metrics = painter.fontMetrics();
    const int leftMargin =
        metrics.horizontalAdvance(format(axisMax)) + 2 * kLabelGap;
    plotRect_ = QRectF(leftMargin, kTopMargin,
                       width() - leftMargin - kRightMargin,
                       height() - kTopMargin - kBottomMargin);
    if (plotRect_.width() <= 0.0 || plotRect_.height() <= 0.0)
    {
      plotRect_ = {};
      slotWidth_ = 0.0;
      return;
    }

    slotWidth_ = plotRect_.width() / static_cast<int>(data_.size());
    paintGrid(painter, plotRect_, leftMargin, axisMax, metrics, palette());
    painter.setPen(QPen(palette().color(QPalette::Dark), 1));
    painter.drawLine(QPointF(plotRect_.left(), plotRect_.bottom()),
                     QPointF(plotRect_.right(), plotRect_.bottom()));
    painter.drawLine(QPointF(plotRect_.left(), plotRect_.top()),
                     QPointF(plotRect_.left(), plotRect_.bottom()));
    paintBars(painter, data_, plotRect_, axisMax, slotWidth_, hoverIndex_,
              palette());
    paintHover(painter, data_, plotRect_, axisMax, slotWidth_, hoverIndex_,
               metrics, palette());
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
