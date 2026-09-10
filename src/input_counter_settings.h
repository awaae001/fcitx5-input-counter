// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_INPUT_COUNTER_SETTINGS_H
#define FCITX5_INPUT_COUNTER_INPUT_COUNTER_SETTINGS_H

//! Owns the Fcitx addon configuration and its persistence.

#include <fcitx-config/configuration.h>
#include <fcitx-config/option.h>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/i18n.h>

namespace inputcounter {

template <typename T>
using TooltipOption =
    fcitx::Option<T, fcitx::NoConstrain<T>, fcitx::DefaultMarshaller<T>,
                  fcitx::ToolTipAnnotation>;

FCITX_CONFIGURATION(
    InputCounterConfig,
    fcitx::Option<bool> quickCounter{this, "QuickCounter", _("Quick counter"),
                                     true};
    TooltipOption<bool> steamGameFilter{
        this,
        "SteamGameFilter",
        _("Filter repeated keys"),
        true,
        {},
        {},
        {_("Filter held-key counts in game input contexts; input method "
           "commits still count.")}};
    TooltipOption<std::string> steamGameIds{
        this,
        "SteamGameIds",
        _("Steam game AppIDs"),
        "",
        {},
        {},
        {_("Separate AppIDs with commas or whitespace; leave empty to select "
           "all games.")}};
    TooltipOption<bool> steamUnknownProgramFallback{
        this,
        "SteamUnknownProgramFallback",
        _("Steam status fallback"),
        true,
        {},
        {},
        {_("Use Steam game running status when the input program cannot be "
           "identified.")}};);

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

  bool steamGameFilterEnabled() const noexcept {
    return *config_.steamGameFilter;
  }
  const std::string &steamGameIds() const noexcept {
    return *config_.steamGameIds;
  }
  bool steamUnknownProgramFallback() const noexcept {
    return *config_.steamUnknownProgramFallback;
  }

private:
  InputCounterConfig config_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_INPUT_COUNTER_SETTINGS_H
