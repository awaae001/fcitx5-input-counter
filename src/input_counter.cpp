// SPDX-License-Identifier: MIT

//! Implements event handling, character counting, and hourly persistence.

#include "input_counter.h"

#include <ctime>
#include <exception>
#include <stdexcept>
#include <vector>

#include <fcitx-utils/cutf8.h>
#include <fcitx-utils/event.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/misc.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/addonmanager.h>
#include <fcitx/event.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterfacemanager.h>

#include "stats_db.h"

namespace inputcounter {

namespace {

/// How often pending counts are written to the database, in microseconds.
constexpr std::uint64_t kFlushIntervalUsec = 60 * 1000 * 1000;

} // namespace

InputCounterAddon::InputCounterAddon(fcitx::AddonManager *manager)
    : instance_(manager == nullptr ? nullptr : manager->instance()) {
  if (instance_ == nullptr) {
    throw std::invalid_argument("inputcounter requires a live Fcitx instance");
  }

  try {
    db_ = std::make_unique<StatsDb>(statsDatabasePath());
  } catch (const std::exception &error) {
    FCITX_ERROR() << "inputcounter could not open the statistics database: "
                  << error.what();
  }

  action_.setIcon("view-statistics");
  action_.setShortText(_("Input statistics"));
  action_.setLongText(_("Committed characters are counted by hour and stored "
                        "locally; no text is retained."));
  action_.connect<fcitx::SimpleAction::Activated>(
      [this](fcitx::InputContext *) {
        flush();
        fcitx::startProcess({INPUT_COUNTER_VIEWER_PATH});
      });
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

  flushEvent_ = instance_->eventLoop().addTimeEvent(
      CLOCK_MONOTONIC, fcitx::now(CLOCK_MONOTONIC) + kFlushIntervalUsec,
      kFlushIntervalUsec, [this](fcitx::EventSourceTime *, std::uint64_t) {
        flush();
        flushEvent_->setNextInterval(kFlushIntervalUsec);
        return true;
      });
}

InputCounterAddon::~InputCounterAddon() { flush(); }

void InputCounterAddon::count(std::string_view text) {
  const auto added =
      text.empty() ? 0 : fcitx_utf8_strnlen_validated(text.data(), text.size());
  if (added == fcitx::utf8::INVALID_LENGTH) {
    FCITX_WARN() << "inputcounter ignored invalid text";
    return;
  }

  const auto hour =
      hourStartOf(static_cast<std::int64_t>(std::time(nullptr)));
  pendingChars_[hour] += static_cast<std::uint64_t>(added);
}

void InputCounterAddon::flush() {
  if (db_ == nullptr || pendingChars_.empty()) {
    return;
  }

  try {
    for (const auto &[hour, chars] : pendingChars_) {
      db_->addChars(hour, chars);
    }
    pendingChars_.clear();
  } catch (const std::exception &error) {
    FCITX_WARN() << "inputcounter failed to persist statistics: "
                 << error.what();
  }
}

} // namespace inputcounter
