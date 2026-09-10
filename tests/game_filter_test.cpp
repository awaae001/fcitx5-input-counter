// SPDX-License-Identifier: MIT
#include "game_key_filter.h"
#include "steam_games.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

using namespace inputcounter;
void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
int main() {
  GameKeyFilter filter;
  filter.press(1, 1, false, 0);
  require(filter.release(1, 1000) == 1, "short tap lost");
  filter.press(1, 1, false, 2000);
  for (std::uint64_t t = 3000; t < 100000; t += 1000)
    filter.press(1, 1, false, t);
  require(filter.release(1, 100000) == 0, "held key counted");
  filter.press(1, 1, true, 200000);
  require(filter.release(1, 201000) == 0, "orphan repeat counted");
  filter.press(1, 1, false, 300000);
  filter.press(2, 1, false, 300001);
  filter.press(1, 1, true, 300002);
  require(filter.release(2, 300003) == 1, "overlapping short tap lost");
  require(filter.release(1, 300004) == 0, "overlapping repeat counted");
  filter.press(1, 1, false, 400000);
  require(filter.release(1, 400000 + GameKeyFilter::timeoutUsec) == 0,
          "expired candidate counted");
  filter.press(1, 1, false, 3000000);
  filter.clear();
  require(filter.release(1, 3000001) == 0, "context reset leaked candidate");
  for (int i = 0; i < 100; ++i)
    filter.press(i, 1, false, 4000000);
  require(filter.release(99, 4000001) == 0, "capacity not bounded");
  filter.clear();
  for (int i = 0; i < 10; ++i) {
    filter.press(1, 1, false, 5000000 + i * 2);
    require(filter.release(1, 5000001 + i * 2) == 1,
            "separate same-key taps classified as hold");
  }

  const char env[] = "X=abc\0SteamAppId=413150\0SteamGameId=999\0";
  require(steamIdsFromEnvironment(std::string_view(env, sizeof(env) - 1)) ==
              std::set<std::string>{"413150"},
          "AppID detection failed");
  const char invalid[] = "SteamAppId=0\0SteamGameId=../../bad\0";
  require(
      steamIdsFromEnvironment(std::string_view(invalid, sizeof(invalid) - 1))
          .empty(),
      "invalid AppID accepted");
  require(steamIdsFromEnvironment("SteamAppId=413150").empty(),
          "truncated environment accepted");
  const char fallback[] = "SteamGameId=413150\0";
  require(steamIdsFromEnvironment(
              std::string_view(fallback, sizeof(fallback) - 1)) ==
              std::set<std::string>{"413150"},
          "GameID fallback failed");
  require(matchesSteamGames({"413150"}, ""),
          "empty list should match all games");
  require(matchesSteamGames({"413150"}, "10, 413150"),
          "configured game missed");
  require(!matchesSteamGames({"413150"}, "41315"), "partial AppID matched");
  require(!matchesSteamGames({}, ""), "no game should not filter");

  const std::set<std::string> programs{"StardewModdingAPI"};
  require(shouldFilterGameKeys("StardewModdingAPI", programs, true, true, true),
          "known game not filtered");
  require(
      shouldFilterGameKeys("StardewModdingAPI", programs, true, false, false),
      "known mapping should work without Steam detection");
  require(!shouldFilterGameKeys("zen-browser", programs, true, true, true),
          "background game affected known non-game program");
  require(!shouldFilterGameKeys("StardewModdingAPI-helper", programs, true,
                                true, true),
          "program matched by substring");
  require(shouldFilterGameKeys("", programs, true, true, true),
          "unknown program fallback failed");
  require(!shouldFilterGameKeys("", programs, true, true, false),
          "disabled fallback still filtered");
  require(!shouldFilterGameKeys("", programs, true, false, true),
          "unknown program filtered without game");
  require(
      !shouldFilterGameKeys("StardewModdingAPI", programs, false, true, true),
      "master switch ignored");
  require(splitGameList("StardewModdingAPI, example\nexample") ==
              std::set<std::string>{"StardewModdingAPI", "example"},
          "program list parsing failed");

  auto path =
      (std::filesystem::temp_directory_path() / "inputcounter-proc-XXXXXX")
          .string();
  require(::mkdtemp(path.data()) != nullptr, "mkdtemp failed");
  try {
    std::filesystem::create_directory(std::filesystem::path(path) / "123");
    std::filesystem::create_directory(std::filesystem::path(path) / "124");
    std::ofstream stream(std::filesystem::path(path) / "123/environ",
                         std::ios::binary);
    stream.write(env, sizeof(env) - 1);
    stream.close();
    require(runningSteamGames(path) == std::set<std::string>{"413150"},
            "process scan failed");
    require(runningSteamGames(std::filesystem::path(path) / "missing").empty(),
            "unavailable proc should return empty");
  } catch (...) {
    std::filesystem::remove_all(path);
    throw;
  }
  std::filesystem::remove_all(path);
  std::cout << "game filter tests passed\n";
}
