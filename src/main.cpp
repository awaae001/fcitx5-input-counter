// SPDX-License-Identifier: MIT

//! Exports the shared-library entry point expected by Fcitx.

#include "input_counter.h"

#include <fcitx-utils/i18n.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>

namespace inputcounter {

/// Creates input-counter addon instances for Fcitx.
class InputCounterAddonFactory final : public fcitx::AddonFactory {
public:
  /// Returns a newly allocated addon instance owned by Fcitx.
  fcitx::AddonInstance *create(fcitx::AddonManager *manager) override {
    fcitx::registerDomain("fcitx5-input-counter", FCITX_INSTALL_LOCALEDIR);
    return new InputCounterAddon(manager);
  }
};

} // namespace inputcounter

FCITX_ADDON_FACTORY(inputcounter::InputCounterAddonFactory)
