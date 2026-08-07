// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_INPUT_COUNTER_H
#define FCITX5_INPUT_COUNTER_INPUT_COUNTER_H

//! Declares the Fcitx addon that owns session counting state.

#include <cstdint>
#include <memory>
#include <string_view>

#include <fcitx-utils/handlertable.h>
#include <fcitx/action.h>
#include <fcitx/addoninstance.h>
#include <fcitx/instance.h>

namespace fcitx {
class AddonManager;
}

namespace inputcounter {

/// Counts text handled by Fcitx and exposes the total in its status menu.
class InputCounterAddon final : public fcitx::AddonInstance {
public:
  /// Registers the counter with a live Fcitx instance.
  explicit InputCounterAddon(fcitx::AddonManager *manager);

private:
  void count(std::string_view text);

  fcitx::Instance *instance_;
  std::uint64_t count_ = 0;
  fcitx::SimpleAction action_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>
      contextCreatedWatcher_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>> commitWatcher_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>
      commitWithCursorWatcher_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>> keyWatcher_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_INPUT_COUNTER_H
