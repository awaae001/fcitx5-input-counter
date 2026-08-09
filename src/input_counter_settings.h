// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_INPUT_COUNTER_SETTINGS_H
#define FCITX5_INPUT_COUNTER_INPUT_COUNTER_SETTINGS_H

//! Owns the Fcitx addon configuration and its persistence.

#include <fcitx-config/configuration.h>
#include <fcitx-config/option.h>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/i18n.h>

namespace inputcounter {

FCITX_CONFIGURATION(InputCounterConfig,
                    fcitx::Option<bool> quickCounter{
                        this, "QuickCounter", _("Quick counter"), true};);

/// Loads, exposes, and persists input-counter settings.
class InputCounterSettings final {
public:
  /// Loads settings from the addon configuration file.
  InputCounterSettings();

  /// Reloads settings from the addon configuration file.
  void reload();

  /// Applies and persists settings received from Fcitx.
  void set(const fcitx::RawConfig &config);

  /// Returns the configuration exposed through the Fcitx configuration UI.
  const fcitx::Configuration *configuration() const noexcept {
    return &config_;
  }

  /// Returns whether the status-area quick counter is enabled.
  bool quickCounterEnabled() const noexcept { return *config_.quickCounter; }

private:
  InputCounterConfig config_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_INPUT_COUNTER_SETTINGS_H
