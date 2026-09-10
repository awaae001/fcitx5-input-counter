// SPDX-License-Identifier: MIT

//! Adapts Fcitx events to counting, persistence, and presentation components.

#include "input_counter.h"

#include <ctime>
#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>

#include <fcitx-utils/cutf8.h>
#include <fcitx-utils/dbus/bus.h>
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

#include <dbus_public.h>
#include <notifications_public.h>

#include "database_manager.h"
#include "input_counter_dbus.h"
#include "statistics_backend.h"
#include "steam_games.h"

namespace inputcounter
{

  namespace
  {

    /// How often pending counts are written to the database, in microseconds.
    constexpr std::uint64_t kFlushIntervalUsec = 60 * 1000 * 1000;
    constexpr std::uint64_t kGamePollIntervalUsec = 3 * 1000 * 1000;

    std::uint64_t physicalKey(const fcitx::KeyEvent &event)
    {
      const auto key = event.rawKey();
      return key.code() != 0 ? (std::uint64_t{1} << 32) |
                                   static_cast<std::uint32_t>(key.code())
                             : static_cast<std::uint32_t>(key.sym());
    }

    fcitx::Instance *requireInstance(fcitx::AddonManager *manager)
    {
      auto *instance = manager == nullptr ? nullptr : manager->instance();
      if (instance == nullptr)
      {
        throw std::invalid_argument("inputcounter requires a live Fcitx instance");
      }
      return instance;
    }

  } // namespace

  InputCounterAddon::InputCounterAddon(fcitx::AddonManager *manager)
      : instance_(requireInstance(manager))
  {
    settings_.reload();
    try
    {
      auto database = std::make_unique<DatabaseManager>();
      auto statistics = std::make_unique<StatisticsBackend>(*database);
      database_ = std::move(database);
      statistics_ = std::move(statistics);
    }
    catch (const std::exception &error)
    {
      FCITX_ERROR() << "inputcounter could not open the statistics database: "
                    << error.what();
      throw;
    }

    dbusObject_ = std::make_unique<InputCounterDBus>(*statistics_);
    notifications_ = manager->addon("notifications", true);
    if (notifications_ == nullptr)
    {
      throw std::runtime_error(
          "inputcounter could not load the notifications addon");
    }
    auto *dbusAddon = manager->addon("dbus");
    auto *bus = dbusAddon == nullptr ? nullptr
                                     : dbusAddon->call<fcitx::IDBusModule::bus>();
    if (bus == nullptr ||
        !bus->addObjectVTable("/inputcounter", "org.fcitx.Fcitx.InputCounter1",
                              *dbusObject_))
    {
      throw std::runtime_error(
          "inputcounter could not register its D-Bus interface");
    }

    action_.setIcon("view-statistics");
    action_.setShortText(_("Input Counter"));
    action_.setLongText(_("Committed graphemes are counted by hour and stored "
                          "locally; no text is retained."));
    action_.connect<fcitx::SimpleAction::Activated>(
        [this](fcitx::InputContext *)
        {
          flush();
          fcitx::startProcess({INPUT_COUNTER_VIEWER_PATH});
        });

    auto &uiManager = instance_->userInterfaceManager();
    if (!action_.registerAction("inputcounter", &uiManager))
    {
      throw std::runtime_error(
          "inputcounter could not register its status action");
    }

    instance_->inputContextManager().foreach (
        [this](fcitx::InputContext *inputContext)
        {
          addStatusActions(inputContext);
          return true;
        });

    contextCreatedWatcher_ = instance_->watchEvent(
        fcitx::EventType::InputContextCreated, fcitx::EventWatcherPhase::Default,
        [this](fcitx::Event &event)
        {
          auto *inputContext =
              static_cast<fcitx::InputContextEvent &>(event).inputContext();
          addStatusActions(inputContext);
        });

    commitWatcher_ = instance_->watchEvent(
        fcitx::EventType::InputContextCommitString,
        fcitx::EventWatcherPhase::Default, [this](fcitx::Event &event)
        {
        const auto &commitEvent =
            static_cast<fcitx::CommitStringEvent &>(event);
        count(commitEvent.text()); });

    commitWithCursorWatcher_ = instance_->watchEvent(
        fcitx::EventType::InputContextCommitStringWithCursor,
        fcitx::EventWatcherPhase::Default, [this](fcitx::Event &event)
        {
        const auto &commitEvent =
            static_cast<fcitx::CommitStringWithCursorEvent &>(event);
        count(commitEvent.text()); });

    refreshSteamGames();
    gamePollEvent_ = instance_->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, fcitx::now(CLOCK_MONOTONIC) + kGamePollIntervalUsec,
        0, [this](fcitx::EventSourceTime *, std::uint64_t)
        {
          try
          {
            refreshSteamGames();
            gameKeys_.expire(fcitx::now(CLOCK_MONOTONIC));
          }
          catch (const std::exception &error)
          {
            FCITX_WARN() << "inputcounter game poll failed: " << error.what();
          }
          gamePollEvent_->setNextInterval(kGamePollIntervalUsec);
          // sd-event time sources are one-shot; re-arm after each fire.
          gamePollEvent_->setOneShot();
          return true; });
    for (const auto type : {fcitx::EventType::InputContextFocusOut,
                            fcitx::EventType::InputContextDestroyed,
                            fcitx::EventType::InputContextReset,
                            fcitx::EventType::InputContextSwitchInputMethod})
    {
      gameContextWatchers_.push_back(instance_->watchEvent(
          type, fcitx::EventWatcherPhase::PreInputMethod,
          [this](fcitx::Event &event)
          {
            if (static_cast<fcitx::InputContextEvent &>(event).inputContext() ==
                gameKeyContext_)
              clearGameKeys();
          }));
    }
    keyReleaseWatcher_ = instance_->watchEvent(
        fcitx::EventType::InputContextKeyEvent,
        fcitx::EventWatcherPhase::PreInputMethod, [this](fcitx::Event &event)
        {
          const auto &keyEvent = static_cast<fcitx::KeyEvent &>(event);
          if (keyEvent.isRelease() &&
              keyEvent.inputContext() == gameKeyContext_) {
            const auto chars = gameKeys_.release(physicalKey(keyEvent),
                                                 fcitx::now(CLOCK_MONOTONIC));
            if (filterGameKeys(keyEvent.inputContext()))
              recordChars(chars);
          } });

    keyWatcher_ = instance_->watchEvent(
        fcitx::EventType::InputContextKeyEvent,
        fcitx::EventWatcherPhase::PostInputMethod, [this](fcitx::Event &event)
        {
        const auto &keyEvent = static_cast<fcitx::KeyEvent &>(event);
        const auto key = keyEvent.key();
        if (keyEvent.isRelease() || keyEvent.filtered()) {
          return;
        }
        if (const auto text = TextCounter::textForKey(key); text.has_value()) {
          if (!filterGameKeys(keyEvent.inputContext())) {
            clearGameKeys();
            count(*text);
          } else {
            if (gameKeyContext_ != keyEvent.inputContext()) {
              clearGameKeys();
              gameKeyContext_ = keyEvent.inputContext();
            }
            gameKeys_.press(physicalKey(keyEvent), textCounter_.count(*text),
                            keyEvent.rawKey().states().test(fcitx::KeyState::Repeat),
                            fcitx::now(CLOCK_MONOTONIC));
          }
        } });

    flushEvent_ = instance_->eventLoop().addTimeEvent(
        CLOCK_MONOTONIC, fcitx::now(CLOCK_MONOTONIC) + kFlushIntervalUsec,
        kFlushIntervalUsec, [this](fcitx::EventSourceTime *, std::uint64_t)
        {
        flush();
        flushEvent_->setNextInterval(kFlushIntervalUsec);
        flushEvent_->setOneShot();
        return true; });
  }

  InputCounterAddon::~InputCounterAddon()
  {
    *notificationCallbacksAlive_ = false;
    if (notifications_ != nullptr)
    {
      for (const auto &[id, notification] : gamePromptNotifications_)
      {
        (void)id;
        notifications_->call<fcitx::INotifications::closeNotification>(
            notification);
      }
    }
    flush();
  }

  void InputCounterAddon::reloadConfig()
  {
    settings_.reload();
    clearGameKeys();
    refreshSteamGames();
  }

  void InputCounterAddon::setConfig(const fcitx::RawConfig &config)
  {
    settings_.set(config);
    clearGameKeys();
    refreshSteamGames();
  }

  void InputCounterAddon::count(std::string_view text)
  {
    const auto codePoints =
        text.empty() ? 0 : fcitx_utf8_strnlen_validated(text.data(), text.size());
    if (codePoints == fcitx::utf8::INVALID_LENGTH)
    {
      FCITX_WARN() << "inputcounter ignored invalid text";
      return;
    }
    if (codePoints == 0)
    {
      return;
    }

    const auto chars = static_cast<std::uint64_t>(textCounter_.count(text));
    recordChars(chars);
  }

  void InputCounterAddon::recordChars(std::uint64_t chars)
  {
    if (chars == 0)
      return;
    database_->recordChars(static_cast<std::int64_t>(std::time(nullptr)), chars);
  }

  void InputCounterAddon::clearGameKeys()
  {
    gameKeys_.clear();
    gameKeyContext_ = nullptr;
  }

  void InputCounterAddon::refreshSteamGames()
  {
    const auto detected = runningSteamGames();
    for (const auto &id : detected)
    {
      if (detectedSteamGames_.count(id) == 0)
        notifySteamGameStarted(id, steamGameName(id));
    }
    detectedSteamGames_ = detected;
    std::set<std::string> running;
    if (*settings_.steamGameFilter)
    {
      for (const auto &id : detected)
      {
        if (settings_.steamGameConfirmed(id) &&
            !settings_.steamGameIgnored(id))
          running.insert(id);
      }
    }
    const bool active = matchesSteamGames(running, *settings_.steamGameIds);
    if (active != steamGameRunning_)
    {
      clearGameKeys();
      steamGameRunning_ = active;
    }
    bool changed = false;
    for (const auto &id : detected)
    {
      changed |= settings_.addSteamGame(id, steamGameName(id));
      if (*settings_.steamGameFilter &&
          !settings_.steamGameConfirmed(id) &&
          !settings_.steamGameIgnored(id))
      {
        promptForSteamGame(id, settings_.knownSteamGameName(id));
      }
    }
    auto programs = settings_.steamGamePrograms();
    if (programs != gamePrograms_)
    {
      clearGameKeys();
      gamePrograms_ = std::move(programs);
    }
    if (changed && !settings_.saveKnownSteamGames())
    {
      FCITX_WARN() << "inputcounter could not save Steam game list";
      settings_.reloadKnownSteamGames();
    }
  }

  void InputCounterAddon::notifySteamGameStarted(const std::string &id,
                                                 const std::string &name)
  {
    if (notifications_ == nullptr)
    {
      FCITX_WARN() << "inputcounter cannot notify game start: notifications "
                      "addon unavailable";
      return;
    }
    const auto displayName = name.empty() ? std::string("Steam App ") + id
                                          : name;
    std::string body = _("Detected that %1 (AppID: %2) has started.");
    const auto first = body.find("%1");
    if (first != std::string::npos)
      body.replace(first, 2, displayName);
    const auto second = body.find("%2");
    if (second != std::string::npos)
      body.replace(second, 2, id);
    notifications_->call<fcitx::INotifications::sendNotification>(
        "Input Counter", 0, "fcitx5-input-counter", _("Game detected"),
        body, std::vector<std::string>{}, 5000, [](const std::string &) {},
        [](std::uint32_t) {});
  }

  void InputCounterAddon::promptForSteamGame(const std::string &id,
                                             const std::string &name)
  {
    if (pendingGamePrompts_.count(id) != 0)
      return;
    if (notifications_ == nullptr)
    {
      FCITX_WARN() << "inputcounter cannot prompt for game verdict: "
                      "notifications addon unavailable";
      return;
    }
    pendingGamePrompts_.insert(id);
    const auto displayName = name.empty() ? std::string("Steam App ") + id
                                          : name;
    std::string body = _(
        "Steam detected %1 (AppID: %2). Focus the game before choosing Yes "
        "so its program name can be recorded.");
    const auto first = body.find("%1");
    if (first != std::string::npos)
      body.replace(first, 2, displayName);
    const auto second = body.find("%2");
    if (second != std::string::npos)
      body.replace(second, 2, id);
    const std::vector<std::string> actions{
        "yes", _("Yes, this is a game"), "no", _("No, ignore")};
    const auto alive = notificationCallbacksAlive_;
    const auto notification =
        notifications_->call<fcitx::INotifications::sendNotification>(
            "Input Counter", 0, "fcitx5-input-counter", _("Is this a game?"),
            body, actions, 0,
            [this, alive, id](const std::string &action)
            {
              if (!*alive)
                return;
              if (action == "yes")
                confirmSteamGame(id);
              else if (action == "no")
                ignoreSteamGame(id);
            },
            [this, alive, id](std::uint32_t)
            {
              if (!*alive)
                return;
              pendingGamePrompts_.erase(id);
              gamePromptNotifications_.erase(id);
            });
    gamePromptNotifications_[id] = notification;
  }

  void InputCounterAddon::confirmSteamGame(const std::string &id)
  {
    auto *inputContext = instance_->mostRecentInputContext();
    const auto program =
        inputContext == nullptr ? std::string{} : inputContext->program();
    settings_.confirmSteamGame(id, program);
    if (!settings_.saveKnownSteamGames())
      FCITX_WARN() << "inputcounter could not save confirmed Steam game";
    pendingGamePrompts_.erase(id);
    gamePromptNotifications_.erase(id);
    clearGameKeys();
    refreshSteamGames();
  }

  void InputCounterAddon::ignoreSteamGame(const std::string &id)
  {
    settings_.ignoreSteamGame(id);
    if (!settings_.saveKnownSteamGames())
      FCITX_WARN() << "inputcounter could not save ignored Steam game";
    pendingGamePrompts_.erase(id);
    gamePromptNotifications_.erase(id);
    clearGameKeys();
    refreshSteamGames();
  }

  bool InputCounterAddon::filterGameKeys(fcitx::InputContext *inputContext) const
  {
    return shouldFilterGameKeys(inputContext->program(), gamePrograms_,
                                *settings_.steamGameFilter, steamGameRunning_,
                                *settings_.steamUnknownProgramFallback);
  }

  void InputCounterAddon::flush()
  {
    try
    {
      database_->flush();
    }
    catch (const std::exception &error)
    {
      FCITX_WARN() << "inputcounter failed to persist statistics: "
                   << error.what();
    }
  }

  void InputCounterAddon::addStatusActions(fcitx::InputContext *inputContext)
  {
    auto &statusArea = inputContext->statusArea();
    statusArea.addAction(fcitx::StatusGroup::AfterInputMethod, &action_);
  }

} // namespace inputcounter
