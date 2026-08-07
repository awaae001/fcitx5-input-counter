// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_INPUT_COUNTER_H
#define FCITX5_INPUT_COUNTER_INPUT_COUNTER_H

//! Declares the Fcitx addon that owns counting and persistence state.

#include <cstdint>
#include <map>
#include <memory>
#include <string_view>

#include <fcitx-utils/eventloopinterface.h>
#include <fcitx-utils/handlertable.h>
#include <fcitx/action.h>
#include <fcitx/addoninstance.h>
#include <fcitx/instance.h>

namespace fcitx {
class AddonManager;
}

namespace inputcounter {

class StatsDb;

/// Counts text handled by Fcitx, persists hourly totals in SQLite, and
/// launches the statistics viewer from its status-area button.
class InputCounterAddon final : public fcitx::AddonInstance {
public:
  /// Registers the counter with a live Fcitx instance.
  explicit InputCounterAddon(fcitx::AddonManager *manager);
  ~InputCounterAddon() override;

private:
  void count(std::string_view text);
  void flush();

  fcitx::Instance *instance_;
  std::unique_ptr<StatsDb> db_;
  std::map<std::int64_t, std::uint64_t> pendingChars_;
  fcitx::SimpleAction action_;
  std::unique_ptr<fcitx::EventSourceTime> flushEvent_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>
      contextCreatedWatcher_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>> commitWatcher_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>
      commitWithCursorWatcher_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>> keyWatcher_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_INPUT_COUNTER_H
