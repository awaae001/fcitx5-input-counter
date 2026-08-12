// SPDX-License-Identifier: MIT

//! Implements the modal editor for a custom chart range.

#include "custom_range_dialog.h"

#include <utility>
#include <variant>

#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QTime>
#include <QVBoxLayout>

#include "i18n.h"

namespace inputcounter {

std::optional<ChartRange> chooseCustomRange(QWidget &parent,
                                            const ChartRange *current) {
  QDialog dialog(&parent);
  dialog.setWindowTitle(IC_("Custom time range"));
  dialog.setModal(true);
  dialog.setMinimumWidth(420);

  auto *layout = new QVBoxLayout(&dialog);
  auto *form = new QFormLayout;
  auto *start = new QDateTimeEdit(&dialog);
  auto *end = new QDateTimeEdit(&dialog);
  for (auto *edit : {start, end}) {
    edit->setCalendarPopup(true);
    edit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
  }

  auto *scale = new QComboBox(&dialog);
  scale->addItem(IC_("1 hour"), static_cast<int>(ChartScale::OneHour));
  scale->addItem(IC_("6 hours"), static_cast<int>(ChartScale::SixHours));
  scale->addItem(IC_("12 hours"), static_cast<int>(ChartScale::TwelveHours));
  scale->addItem(IC_("1 day"), static_cast<int>(ChartScale::OneDay));
  scale->addItem(IC_("1 week"), static_cast<int>(ChartScale::OneWeek));
  scale->addItem(IC_("1 month"), static_cast<int>(ChartScale::OneMonth));
  if (current != nullptr) {
    start->setDateTime(current->start());
    end->setDateTime(current->end());
    scale->setCurrentIndex(
        scale->findData(static_cast<int>(current->scale())));
  } else {
    const auto now = QDateTime::currentDateTime();
    auto rangeEnd = now;
    rangeEnd.setTime(QTime(now.time().hour(), 0));
    if (rangeEnd < now) {
      rangeEnd = rangeEnd.addSecs(60 * 60);
    }
    start->setDateTime(rangeEnd.addDays(-7));
    end->setDateTime(rangeEnd);
    scale->setCurrentIndex(
        scale->findData(static_cast<int>(ChartScale::SixHours)));
  }
  form->addRow(IC_("Start time"), start);
  form->addRow(IC_("End time"), end);
  form->addRow(IC_("Time scale"), scale);
  auto *bucketCount = new QLabel(&dialog);
  form->addRow(IC_("Bars"), bucketCount);
  layout->addLayout(form);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                           QDialogButtonBox::Cancel,
                                       &dialog);
  auto *applyButton = buttons->button(QDialogButtonBox::Ok);
  applyButton->setText(IC_("Apply"));
  layout->addWidget(buttons);

  const auto updateBucketCount = [&] {
    const auto selectedScale =
        static_cast<ChartScale>(scale->currentData().toInt());
    auto result = ChartRange::create(start->dateTime(), end->dateTime(),
                                     selectedScale);
    if (const auto *rangeError = std::get_if<ChartRangeError>(&result)) {
      if (*rangeError == ChartRangeError::TooManyBuckets) {
        bucketCount->setText(
            QString(IC_("%1 / %2 (over limit)"))
                .arg(ChartRange::countBuckets(start->dateTime(),
                                              end->dateTime(), selectedScale))
                .arg(ChartRange::kMaximumBuckets));
      } else {
        bucketCount->setText(IC_("Invalid range"));
      }
      bucketCount->setStyleSheet(
          QStringLiteral("color: #da4453; font-weight: 600;"));
      applyButton->setEnabled(false);
      return;
    }

    bucketCount->setText(QStringLiteral("%1 / %2")
                             .arg(std::get<ChartRange>(result).buckets().size())
                             .arg(ChartRange::kMaximumBuckets));
    bucketCount->setStyleSheet(QString());
    applyButton->setEnabled(true);
  };

  QObject::connect(start, &QDateTimeEdit::dateTimeChanged, &dialog,
                   [&](const QDateTime &) { updateBucketCount(); });
  QObject::connect(end, &QDateTimeEdit::dateTimeChanged, &dialog,
                   [&](const QDateTime &) { updateBucketCount(); });
  QObject::connect(scale, &QComboBox::currentIndexChanged, &dialog,
                   [&](int) { updateBucketCount(); });

  std::optional<ChartRange> selected;
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog,
                   &QDialog::reject);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
    const auto selectedScale =
        static_cast<ChartScale>(scale->currentData().toInt());
    auto result = ChartRange::create(start->dateTime(), end->dateTime(),
                                     selectedScale);
    if (std::holds_alternative<ChartRangeError>(result)) {
      updateBucketCount();
      return;
    }

    selected.emplace(std::move(std::get<ChartRange>(result)));
    dialog.accept();
  });
  updateBucketCount();

  dialog.exec();
  return selected;
}

} // namespace inputcounter
