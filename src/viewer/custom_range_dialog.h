// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_VIEWER_CUSTOM_RANGE_DIALOG_H
#define FCITX5_INPUT_COUNTER_VIEWER_CUSTOM_RANGE_DIALOG_H

//! Declares the modal editor for a custom chart range.

#include <optional>

#include "chart_range.h"

class QWidget;

namespace inputcounter {

/// Opens a modal range editor and returns the validated selection.
///
/// current supplies the initial values. Passing nullptr selects the last seven
/// days with a six-hour scale. Closing or cancelling returns std::nullopt.
std::optional<ChartRange> chooseCustomRange(QWidget &parent,
                                            const ChartRange *current);

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_VIEWER_CUSTOM_RANGE_DIALOG_H
