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
      [this](fcitx::InputContext *) { toggleRecording(); });

  resetAction_.setShortText(_("Reset"));
  resetAction_.connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext *) { reset(); });

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

  updateActions();
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
    updateActions();
  }

  instance_.inputContextManager().foreach (
      [this](fcitx::InputContext *inputContext) {
        detach(*inputContext);
        attach(*inputContext);
        return true;
      });
}

void QuickCounter::record(std::uint64_t chars) {
  if (!recording_) {
    return;
  }

  chars_ += chars;
  updateActions();
}

void QuickCounter::detach(fcitx::InputContext &inputContext) {
  auto &statusArea = inputContext.statusArea();
  statusArea.removeAction(&separatorAction_);
  statusArea.removeAction(&countAction_);
  statusArea.removeAction(&toggleRecordingAction_);
  statusArea.removeAction(&resetAction_);
}

void QuickCounter::toggleRecording() {
  recording_ = !recording_;
  updateActions();
}

void QuickCounter::reset() {
  chars_ = 0;
  updateActions();
}

void QuickCounter::updateActions() {
  std::string label = _("Count");
  label += ": ";
  label += std::to_string(chars_);
  countAction_.setShortText(label);

  toggleRecordingAction_.setShortText(recording_ ? _("Pause recording")
                                                 : _("Start recording"));

  instance_.inputContextManager().foreach (
      [this](fcitx::InputContext *inputContext) {
        countAction_.update(inputContext);
        toggleRecordingAction_.update(inputContext);
        return true;
      });
}

} // namespace inputcounter
