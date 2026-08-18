// SPDX-License-Identifier: MIT

//! Implements the quick counter session and status-area actions.

#include "quick_counter.h"

#include <stdexcept>
#include <string>

#include <fcitx-utils/i18n.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/instance.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterfacemanager.h>

namespace inputcounter {

QuickCounter::QuickCounter(fcitx::Instance &instance) : instance_(instance) {
  separatorAction_.setSeparator(true);

  toggleRecordingAction_.connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext *) {
        recording_ = !recording_;
        updateRecordingText();
        updateAll(toggleRecordingAction_);
      });

  resetAction_.setShortText(_("Reset"));
  resetAction_.connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext *) {
        chars_ = 0;
        updateCountText();
        updateAll(countAction_);
      });

  updateCountText();
  updateRecordingText();

  auto &uiManager = instance_.userInterfaceManager();
  if (!separatorAction_.registerAction("inputcounter-session-separator",
                                       &uiManager) ||
      !countAction_.registerAction("inputcounter-session-count", &uiManager) ||
      !toggleRecordingAction_.registerAction("inputcounter-toggle-recording",
                                             &uiManager) ||
      !resetAction_.registerAction("inputcounter-reset-session", &uiManager)) {
    throw std::runtime_error(
        "inputcounter could not register its menu actions");
  }
}

void QuickCounter::attach(fcitx::InputContext &inputContext) {
  if (!visible_) {
    return;
  }

  auto &statusArea = inputContext.statusArea();
  statusArea.addAction(fcitx::StatusGroup::AfterInputMethod, &separatorAction_);
  statusArea.addAction(fcitx::StatusGroup::AfterInputMethod, &countAction_);
  statusArea.addAction(fcitx::StatusGroup::AfterInputMethod,
                       &toggleRecordingAction_);
  statusArea.addAction(fcitx::StatusGroup::AfterInputMethod, &resetAction_);
}

void QuickCounter::setVisible(bool visible) {
  if (visible_ == visible) {
    return;
  }

  visible_ = visible;
  if (!visible_) {
    recording_ = false;
    updateRecordingText();
  }

  instance_.inputContextManager().foreach (
      [this](fcitx::InputContext *inputContext) {
        auto &statusArea = inputContext->statusArea();
        statusArea.removeAction(&separatorAction_);
        statusArea.removeAction(&countAction_);
        statusArea.removeAction(&toggleRecordingAction_);
        statusArea.removeAction(&resetAction_);
        attach(*inputContext);
        return true;
      });
}

void QuickCounter::record(std::uint64_t chars,
                          fcitx::InputContext *inputContext) {
  if (!recording_) {
    return;
  }

  chars_ += chars;
  updateCountText();
  if (inputContext != nullptr) {
    countAction_.update(inputContext);
  }
}

void QuickCounter::updateCountText() {
  std::string label = _("Count");
  label += ": ";
  label += std::to_string(chars_);
  countAction_.setShortText(label);
}

void QuickCounter::updateRecordingText() {
  toggleRecordingAction_.setShortText(recording_ ? _("Pause recording")
                                                 : _("Start recording"));
}

void QuickCounter::updateAll(fcitx::SimpleAction &action) {
  instance_.inputContextManager().foreach (
      [&action](fcitx::InputContext *inputContext) {
        action.update(inputContext);
        return true;
      });
}

} // namespace inputcounter
