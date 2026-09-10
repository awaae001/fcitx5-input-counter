// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_INPUT_COUNTER_SETTINGS_H
#define FCITX5_INPUT_COUNTER_INPUT_COUNTER_SETTINGS_H

//! Owns the addon configuration and the persisted Steam game registry.

#include <set>
#include <string>

#include <fcitx-config/configuration.h>
#include <fcitx-config/iniparser.h>
#include <fcitx-config/option.h>
#include <fcitx-config/rawconfig.h>
#include <fcitx-utils/i18n.h>
#include <fcitx-utils/stringutils.h>

#include "steam_games.h"

namespace inputcounter {

template <typename T>
using TooltipOption =
    fcitx::Option<T, fcitx::NoConstrain<T>, fcitx::DefaultMarshaller<T>,
                  fcitx::ToolTipAnnotation>;

/// Fcitx settings plus the separate registry of detected Steam games.
FCITX_CONFIGURATION(
    InputCounterSettings,
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
           "identified.")}};

  void reload() {
    fcitx::readAsIni(*this, kConfigPath);
    reloadKnownSteamGames();
  }

  void set(const fcitx::RawConfig &config) {
    load(config, true);
    fcitx::safeSaveAsIni(*this, kConfigPath);
    reloadKnownSteamGames();
  }

  bool updateSteamGame(const std::string &id, const std::string &name,
                       const std::set<std::string> &programs) {
    bool changed = false;
    if (!knownSteamGames_.get(id)) {
      knownSteamGames_[id + "/Confirmed"] = "False";
      knownSteamGames_[id + "/Ignored"] = "False";
      changed = true;
    }
    const auto currentName = knownSteamGames_.get(id + "/Name");
    if (!name.empty() && (!currentName || currentName->value() != name)) {
      knownSteamGames_[id + "/Name"] = name;
      changed = true;
    }
    const auto programList = fcitx::stringutils::join(programs, ",");
    const auto currentPrograms = knownSteamGames_.get(id + "/Programs");
    if (!currentPrograms || currentPrograms->value() != programList) {
      knownSteamGames_[id + "/Programs"] = programList;
      changed = true;
    }
    return changed;
  }

  std::string knownSteamGameName(const std::string &id) const {
    const auto name = knownSteamGames_.get(id + "/Name");
    return name ? name->value() : std::string{};
  }

  bool steamGameConfirmed(const std::string &id) const { return steamGameFlag(id, "Confirmed"); }

  bool steamGameIgnored(const std::string &id) const { return steamGameFlag(id, "Ignored"); }

  void confirmSteamGame(const std::string &id) {
    knownSteamGames_[id + "/Confirmed"] = "True";
    knownSteamGames_[id + "/Ignored"] = "False";
  }

  void ignoreSteamGame(const std::string &id) {
    knownSteamGames_[id + "/Confirmed"] = "False";
    knownSteamGames_[id + "/Ignored"] = "True";
  }

  std::set<std::string> steamGamePrograms() const {
    std::set<std::string> programs;
    if (!*steamGameFilter)
      return programs;
    for (const auto &id : knownSteamGames_.subItems()) {
      if (!steamGameConfirmed(id) || steamGameIgnored(id) ||
          !matchesSteamGames({id}, *steamGameIds))
        continue;
      if (const auto value = knownSteamGames_.get(id + "/Programs")) {
        const auto entries = splitGameList(value->value());
        programs.insert(entries.begin(), entries.end());
      }
    }
    return programs;
  }

  bool saveKnownSteamGames() const { return fcitx::safeSaveAsIni(knownSteamGames_, kKnownGamesPath); }

  void reloadKnownSteamGames() {
    knownSteamGames_ = fcitx::RawConfig();
    fcitx::readAsIni(knownSteamGames_, kKnownGamesPath);
  }

private:
  bool steamGameFlag(const std::string &id, const char *name) const {
    const auto value = knownSteamGames_.get(id + "/" + name);
    return value != nullptr && value->value() == "True";
  }

  static constexpr char kConfigPath[] = "conf/inputcounter.conf";
  static constexpr char kKnownGamesPath[] =
      "conf/inputcounter-steam-games.conf";

  fcitx::RawConfig knownSteamGames_;
);

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_INPUT_COUNTER_SETTINGS_H
