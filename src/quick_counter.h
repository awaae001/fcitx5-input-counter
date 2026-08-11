// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_QUICK_COUNTER_H
#define FCITX5_INPUT_COUNTER_QUICK_COUNTER_H

//! Owns the quick counter session and its status-area actions.

#include <cstdint>

#include <fcitx/action.h>

namespace fcitx {
class InputContext;
class Instance;
} // namespace fcitx

namespace inputcounter {

/// Presents an optional, resettable character count for the current session.
class QuickCounter final {
public:
  /// Registers the quick-counter actions with instance.
  ///
  /// Throws std::runtime_error if any action cannot be registered.
  explicit QuickCounter(fcitx::Instance &instance);

  /// Adds the quick-counter actions to inputContext when visible.
  void attach(fcitx::InputContext &inputContext);

  /// Shows or hides the quick counter on every existing input context.
  ///
  /// Hiding the counter also pauses session recording.
  void setVisible(bool visible);

  /// Adds chars to the session count while recording is active.
  void record(std::uint64_t chars);

private:
  void updateActions();

  fcitx::Instance &instance_;
  std::uint64_t chars_ = 0;
  bool recording_ = false;
  bool visible_ = true;
  fcitx::SimpleAction separatorAction_;
  fcitx::SimpleAction countAction_;
  fcitx::SimpleAction toggleRecordingAction_;
  fcitx::SimpleAction resetAction_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_QUICK_COUNTER_H
