// SPDX-License-Identifier: MIT

//! Implements loading and persistence for the addon configuration.

#include "input_counter_settings.h"

#include <fcitx-config/iniparser.h>

namespace inputcounter {

namespace {

constexpr char kConfigPath[] = "conf/inputcounter.conf";

} // namespace

InputCounterSettings::InputCounterSettings() { reload(); }

void InputCounterSettings::reload() { fcitx::readAsIni(config_, kConfigPath); }

void InputCounterSettings::set(const fcitx::RawConfig &config) {
  config_.load(config, true);
  fcitx::safeSaveAsIni(config_, kConfigPath);
}

} // namespace inputcounter
