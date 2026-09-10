// SPDX-License-Identifier: MIT
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
  require(shouldExcludeGameInput("StardewModdingAPI", programs, true, true,
                                 true),
          "game input not excluded");
  require(!shouldExcludeGameInput("StardewModdingAPI", programs, true, false,
                                  true),
          "stopped game excluded input");
  require(!shouldExcludeGameInput("zen-browser", programs, true, true, true),
          "chat input excluded while game was running");
  require(!shouldExcludeGameInput("StardewModdingAPI-helper", programs, true,
                                  true, true),
          "program matched by substring");
  require(shouldExcludeGameInput("", programs, true, true, true),
          "unknown program fallback failed");
  require(!shouldExcludeGameInput("", programs, true, true, false),
          "disabled fallback still filtered");
  require(!shouldExcludeGameInput("", programs, true, false, true),
          "unknown program filtered without game");
  require(
      !shouldExcludeGameInput("StardewModdingAPI", programs, false, true, true),
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
    std::ofstream(std::filesystem::path(path) / "123/comm") << "StardewValley\n";
    const char command[] = "/games/StardewValley\0--launch-option\0";
    std::ofstream commandStream(std::filesystem::path(path) / "123/cmdline",
                                std::ios::binary);
    commandStream.write(command, sizeof(command) - 1);
    commandStream.close();
    std::ofstream secondEnvironment(
        std::filesystem::path(path) / "124/environ", std::ios::binary);
    secondEnvironment.write(env, sizeof(env) - 1);
    secondEnvironment.close();
    std::ofstream(std::filesystem::path(path) / "124/comm")
        << "StardewModdingAPI\n";
    require(runningSteamGames(path) ==
                RunningSteamGames{{"413150",
                                   {"StardewModdingAPI", "StardewValley"}}},
            "multiple game processes were not collected");
    require(runningSteamGames(std::filesystem::path(path) / "missing").empty(),
            "unavailable proc should return empty");
  } catch (...) {
    std::filesystem::remove_all(path);
    throw;
  }
  std::filesystem::remove_all(path);
  std::cout << "game filter tests passed\n";
}
