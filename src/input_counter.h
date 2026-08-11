// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_INPUT_COUNTER_H
#define FCITX5_INPUT_COUNTER_INPUT_COUNTER_H

//! Declares the Fcitx event adapter for input counting.

#include <memory>
#include <string_view>

#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/eventloopinterface.h>
#include <fcitx-utils/handlertable.h>
#include <fcitx/action.h>
#include <fcitx/addoninstance.h>
#include <fcitx/instance.h>

#include "input_counter_settings.h"
#include "quick_counter.h"

namespace fcitx {
class AddonManager;
}

namespace inputcounter {

class DatabaseManager;
class HourlyCountBuffer;
class InputCounterDBus;
class StatisticsBackend;

/// Counts text handled by Fcitx, persists hourly totals in SQLite, and
/// launches the statistics viewer from its status-area button.
class InputCounterAddon final : public fcitx::AddonInstance {
public:
  /// Registers the counter with a live Fcitx instance.
  explicit InputCounterAddon(fcitx::AddonManager *manager);
  ~InputCounterAddon() override;

  /// Reloads addon settings and reapplies quick-counter visibility.
  void reloadConfig() override;
  /// Returns the settings exposed through the Fcitx configuration UI.
  const fcitx::Configuration *getConfig() const override {
    return settings_.configuration();
  }
  /// Applies and persists settings received from the Fcitx configuration UI.
  void setConfig(const fcitx::RawConfig &config) override;

private:
  void count(std::string_view text);
  void flush();
  void addStatusActions(fcitx::InputContext *inputContext);
  void applySettings();
  void openViewer();

  fcitx::Instance *instance_;
  InputCounterSettings settings_;
  std::unique_ptr<DatabaseManager> database_;
  std::unique_ptr<HourlyCountBuffer> hourlyCounts_;
  std::unique_ptr<StatisticsBackend> statistics_;
  QuickCounter quickCounter_;
  fcitx::SimpleAction action_;
  std::unique_ptr<fcitx::EventSourceTime> flushEvent_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>
      contextCreatedWatcher_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>> commitWatcher_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>
      commitWithCursorWatcher_;
  std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>> keyWatcher_;
  std::unique_ptr<InputCounterDBus> dbusObject_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_INPUT_COUNTER_H
