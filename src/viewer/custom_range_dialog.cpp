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

namespace {

QString rangeErrorMessage(ChartRangeError error) {
  switch (error) {
  case ChartRangeError::InvalidTime:
    return IC_("Choose valid start and end times.");
  case ChartRangeError::EndNotAfterStart:
    return IC_("Start time must be earlier than end time.");
  case ChartRangeError::TooManyBuckets:
    return QString(
               IC_("The selected range contains more than %1 bars. Choose a "
                   "coarser time scale or a shorter range."))
        .arg(ChartRange::kMaximumBuckets);
  }
  return {};
}

QDateTime defaultEnd() {
  const auto now = QDateTime::currentDateTime();
  auto end = now;
  end.setTime(QTime(now.time().hour(), 0));
  return end < now ? end.addSecs(60 * 60) : end;
}

void populateScales(QComboBox &scale) {
  scale.addItem(IC_("1 hour"), static_cast<int>(ChartScale::OneHour));
  scale.addItem(IC_("6 hours"), static_cast<int>(ChartScale::SixHours));
  scale.addItem(IC_("12 hours"), static_cast<int>(ChartScale::TwelveHours));
  scale.addItem(IC_("1 day"), static_cast<int>(ChartScale::OneDay));
  scale.addItem(IC_("1 week"), static_cast<int>(ChartScale::OneWeek));
  scale.addItem(IC_("1 month"), static_cast<int>(ChartScale::OneMonth));
}

} // namespace

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
  populateScales(*scale);
  if (current != nullptr) {
    start->setDateTime(current->start());
    end->setDateTime(current->end());
    scale->setCurrentIndex(
        scale->findData(static_cast<int>(current->scale())));
  } else {
    const auto rangeEnd = defaultEnd();
    start->setDateTime(rangeEnd.addDays(-7));
    end->setDateTime(rangeEnd);
    scale->setCurrentIndex(
        scale->findData(static_cast<int>(ChartScale::SixHours)));
  }
  form->addRow(IC_("Start time"), start);
  form->addRow(IC_("End time"), end);
  form->addRow(IC_("Time scale"), scale);
  layout->addLayout(form);

  auto *error = new QLabel(&dialog);
  error->setWordWrap(true);
  error->hide();
  layout->addWidget(error);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                           QDialogButtonBox::Cancel,
                                       &dialog);
  buttons->button(QDialogButtonBox::Ok)->setText(IC_("Apply"));
  layout->addWidget(buttons);

  std::optional<ChartRange> selected;
  QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog,
                   &QDialog::reject);
  QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
    const auto selectedScale =
        static_cast<ChartScale>(scale->currentData().toInt());
    auto result = ChartRange::create(start->dateTime(), end->dateTime(),
                                     selectedScale);
    if (const auto *rangeError = std::get_if<ChartRangeError>(&result)) {
      error->setText(rangeErrorMessage(*rangeError));
      error->show();
      return;
    }

    selected.emplace(std::move(std::get<ChartRange>(result)));
    dialog.accept();
  });

  dialog.exec();
  return selected;
}

} // namespace inputcounter
