// SPDX-License-Identifier: MIT

//! Implements event handling, character counting, and menu updates.

#include "input_counter.h"

#include <limits>
#include <stdexcept>

#include <fcitx-utils/cutf8.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/stringutils.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/addonmanager.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterfacemanager.h>

namespace inputcounter {

InputCounterAddon::InputCounterAddon(fcitx::AddonManager *manager)
    : instance_(manager == nullptr ? nullptr : manager->instance()) {
  if (instance_ == nullptr) {
    throw std::invalid_argument("inputcounter requires a live Fcitx instance");
  }

  action_.setIcon("view-statistics");
  action_.setShortText(fcitx::stringutils::concat(_("Session input: "), count_,
                                                  _(" characters")));
  action_.setLongText(_("Committed characters counted since Fcitx "
                        "started; no text is retained."));
  if (!action_.registerAction("inputcounter",
                              &instance_->userInterfaceManager())) {
    throw std::runtime_error(
        "inputcounter could not register its status action");
  }

  instance_->inputContextManager().foreach (
      [this](fcitx::InputContext *inputContext) {
        inputContext->statusArea().addAction(
            fcitx::StatusGroup::AfterInputMethod, &action_);
        return true;
      });

  contextCreatedWatcher_ = instance_->watchEvent(
      fcitx::EventType::InputContextCreated, fcitx::EventWatcherPhase::Default,
      [this](fcitx::Event &event) {
        auto *inputContext =
            static_cast<fcitx::InputContextEvent &>(event).inputContext();
        inputContext->statusArea().addAction(
            fcitx::StatusGroup::AfterInputMethod, &action_);
      });

  commitWatcher_ = instance_->watchEvent(
      fcitx::EventType::InputContextCommitString,
      fcitx::EventWatcherPhase::Default, [this](fcitx::Event &event) {
        count(static_cast<fcitx::CommitStringEvent &>(event).text());
      });

  commitWithCursorWatcher_ = instance_->watchEvent(
      fcitx::EventType::InputContextCommitStringWithCursor,
      fcitx::EventWatcherPhase::Default, [this](fcitx::Event &event) {
        count(static_cast<fcitx::CommitStringWithCursorEvent &>(event).text());
      });

  keyWatcher_ = instance_->watchEvent(
      fcitx::EventType::InputContextKeyEvent,
      fcitx::EventWatcherPhase::PostInputMethod, [this](fcitx::Event &event) {
        const auto &keyEvent = static_cast<fcitx::KeyEvent &>(event);
        const auto key = keyEvent.key();
        constexpr fcitx::KeyStates shortcutStates{
            fcitx::KeyState::Ctrl,  fcitx::KeyState::Alt,
            fcitx::KeyState::Super, fcitx::KeyState::Super2,
            fcitx::KeyState::Hyper, fcitx::KeyState::Hyper2,
            fcitx::KeyState::Meta};
        if (keyEvent.isRelease() || keyEvent.filtered() || key.isModifier() ||
            key.states().testAny(shortcutStates)) {
          return;
        }
        const auto text = fcitx::Key::keySymToUTF8(key.sym());
        if (!text.empty()) {
          count(text);
        }
      });
}

void InputCounterAddon::count(std::string_view text) {
  const auto added =
      text.empty() ? 0 : fcitx_utf8_strnlen_validated(text.data(), text.size());
  if (added == fcitx::utf8::INVALID_LENGTH ||
      added > std::numeric_limits<std::uint64_t>::max() - count_) {
    FCITX_WARN() << "inputcounter ignored invalid or overflowing text";
    return;
  }

  count_ += added;
  action_.setShortText(fcitx::stringutils::concat(_("Session input: "), count_,
                                                  _(" characters")));
  instance_->inputContextManager().foreach (
      [this](fcitx::InputContext *inputContext) {
        action_.update(inputContext);
        return true;
      });
}

} // namespace inputcounter
