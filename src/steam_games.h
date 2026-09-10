// SPDX-License-Identifier: MIT
#ifndef INPUT_COUNTER_STEAM_GAMES_H
#define INPUT_COUNTER_STEAM_GAMES_H

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <string_view>

namespace inputcounter {
// Local best-effort presence detection, not Steam's network/focus status.
std::set<std::string> steamIdsFromEnvironment(std::string_view environment);
using RunningSteamGames = std::map<std::string, std::set<std::string>>;
RunningSteamGames
runningSteamGames(const std::filesystem::path &proc = "/proc");
bool matchesSteamGames(const std::set<std::string> &running,
                       std::string_view configuredIds);
std::string steamGameName(const std::string &id);
std::set<std::string> splitGameList(std::string_view list);
bool shouldFilterGameKeys(std::string_view program,
                          const std::set<std::string> &gamePrograms,
                          bool enabled, bool running, bool unknownFallback);
} // namespace inputcounter
#endif
