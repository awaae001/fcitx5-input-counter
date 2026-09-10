// SPDX-License-Identifier: MIT
#include "steam_games.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace inputcounter {
namespace {
bool validId(std::string_view id) {
  return !id.empty() && id.size() <= 20 && id.front() != '0' &&
         std::all_of(id.begin(), id.end(),
                     [](char c) { return c >= '0' && c <= '9'; });
}
std::string readBounded(const std::filesystem::path &path, std::size_t limit) {
  std::ifstream stream(path, std::ios::binary);
  std::string result(limit, '\0');
  stream.read(result.data(), static_cast<std::streamsize>(result.size()));
  result.resize(static_cast<std::size_t>(stream.gcount()));
  return result;
}

} // namespace

std::set<std::string> steamIdsFromEnvironment(std::string_view environment) {
  std::string app, game;
  while (!environment.empty()) {
    const auto end = environment.find('\0');
    // Ignore an incomplete final entry (including truncated /proc reads).
    if (end == std::string_view::npos)
      break;
    const auto entry = environment.substr(0, end);
    constexpr std::string_view appPrefix = "SteamAppId=";
    constexpr std::string_view gamePrefix = "SteamGameId=";
    if (entry.substr(0, appPrefix.size()) == appPrefix)
      app = std::string(entry.substr(appPrefix.size()));
    if (entry.substr(0, gamePrefix.size()) == gamePrefix)
      game = std::string(entry.substr(gamePrefix.size()));
    environment.remove_prefix(end + 1);
  }
  // Prefer AppID: GameID may encode additional launch information.
  if (validId(app))
    return {app};
  if (validId(game))
    return {game};
  return {};
}

RunningSteamGames runningSteamGames(const std::filesystem::path &proc) {
  RunningSteamGames result;
  std::error_code error;
  std::filesystem::directory_iterator it(proc, error), end;
  for (; !error && it != end; it.increment(error)) {
    const auto filename = it->path().filename().string();
    if (!validId(filename))
      continue;
    struct stat status{};
    if (::stat(it->path().c_str(), &status) != 0 || status.st_uid != ::getuid())
      continue;
    for (const auto &id :
         steamIdsFromEnvironment(readBounded(it->path() / "environ", 65536))) {
      auto &programs = result[id];
      auto add = [&programs](std::string name) {
        while (!name.empty() && (name.back() == '\0' || name.back() == '\n'))
          name.pop_back();
        if (!name.empty() &&
            name.find_first_of(" \t\r\n,") == std::string::npos)
          programs.insert(std::move(name));
      };
      add(readBounded(it->path() / "comm", 256));
      const auto command = readBounded(it->path() / "cmdline", 4096);
      add(std::filesystem::path(command.substr(0, command.find('\0')))
              .filename()
              .string());
      std::error_code linkError;
      add(std::filesystem::read_symlink(it->path() / "exe", linkError)
              .filename()
              .string());
    }
  }
  return result;
}

bool matchesSteamGames(const std::set<std::string> &running,
                       std::string_view configuredIds) {
  if (configuredIds.find_first_not_of(" \t\r\n") == std::string_view::npos)
    return !running.empty();
  for (const auto &id : splitGameList(configuredIds)) {
    if (running.count(id))
      return true;
  }
  return false;
}

std::set<std::string> splitGameList(std::string_view value) {
  std::string list(value);
  std::replace(list.begin(), list.end(), ',', ' ');
  std::istringstream stream(list);
  std::set<std::string> result;
  for (std::string token; stream >> token;)
    result.insert(token);
  return result;
}

bool shouldExcludeGameInput(std::string_view program,
                            const std::set<std::string> &gamePrograms,
                            bool enabled, bool running, bool unknownFallback) {
  if (!enabled || !running)
    return false;
  if (!program.empty())
    return gamePrograms.count(std::string(program)) != 0;
  return unknownFallback;
}

std::string steamGameName(const std::string &id) {
  if (!validId(id))
    return {};
  const auto *home = std::getenv("HOME");
  if (!home)
    return {};
  std::set<std::filesystem::path> roots{
      std::filesystem::path(home) / ".local/share/Steam",
      std::filesystem::path(home) / ".steam/steam",
      std::filesystem::path(home) /
          ".var/app/com.valvesoftware.Steam/.local/share/Steam"};
  if (const auto *data = std::getenv("XDG_DATA_HOME"); data && *data)
    roots.emplace(std::filesystem::path(data) / "Steam");
  const std::regex pathPattern("\"path\"\\s*\"([^\"]+)\"");
  const auto initialRoots = roots;
  for (const auto &root : initialRoots) {
    const auto libraries =
        readBounded(root / "steamapps/libraryfolders.vdf", 1024 * 1024);
    for (auto it = std::sregex_iterator(libraries.begin(), libraries.end(),
                                        pathPattern);
         it != std::sregex_iterator(); ++it) {
      roots.emplace((*it)[1].str());
    }
  }
  const std::regex namePattern("\"name\"\\s*\"([^\"]+)\"");
  for (const auto &root : roots) {
    const auto manifest =
        readBounded(root / "steamapps" / ("appmanifest_" + id + ".acf"), 65536);
    std::smatch match;
    if (std::regex_search(manifest, match, namePattern))
      return match[1].str();
  }
  return {};
}
} // namespace inputcounter
