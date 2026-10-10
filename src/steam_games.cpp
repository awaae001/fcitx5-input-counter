// SPDX-License-Identifier: MIT
#include "steam_games.h"

#include <algorithm>
#include <charconv>
#include <regex>
#include <sys/stat.h>
#include <unistd.h>

#include <fcitx-utils/charutils.h>
#include <fcitx-utils/environ.h>
#include <fcitx-utils/fs.h>
#include <fcitx-utils/misc.h>
#include <fcitx-utils/standardpaths.h>
#include <fcitx-utils/stringutils.h>

namespace inputcounter
{
  namespace
  {
    bool validId(std::string_view id)
    {
      return !id.empty() && id.size() <= 20 && id.front() != '0' &&
             std::all_of(id.begin(), id.end(), fcitx::charutils::isdigit);
    }
    std::string readBounded(const std::filesystem::path &path, std::size_t limit)
    {
      auto file = fcitx::StandardPaths::openPath(path);
      if (!file.isValid())
        return {};

      std::string result(limit, '\0');
      std::size_t offset = 0;
      while (offset < limit)
      {
        const auto size =
            fcitx::fs::safeRead(file.fd(), result.data() + offset, limit - offset);
        if (size <= 0)
          break;
        offset += static_cast<std::size_t>(size);
      }
      result.resize(offset);
      return result;
    }

  } // namespace

  std::set<std::string> steamIdsFromEnvironment(std::string_view environment)
  {
    std::string app, game;
    const auto completeEnd = environment.rfind('\0');
    if (completeEnd != std::string_view::npos)
    {
      environment = environment.substr(0, completeEnd);
      constexpr std::string_view nullDelimiter{"\0", 1};
      for (const auto &item : fcitx::stringutils::split(
               environment, nullDelimiter,
               fcitx::stringutils::SplitBehavior::SkipEmpty))
      {
        std::string_view entry = item;
        if (fcitx::stringutils::consumePrefix(entry, "SteamAppId="))
          app = entry;
        else if (fcitx::stringutils::consumePrefix(entry, "SteamGameId="))
          game = entry;
      }
    }
    // Prefer AppID: GameID may encode additional launch information.
    if (validId(app))
      return {app};
    if (validId(game))
      return {game};
    return {};
  }

  RunningSteamGames runningSteamGames(const std::filesystem::path &proc)
  {
    RunningSteamGames result;
    std::error_code error;
    std::filesystem::directory_iterator it(proc, error), end;
    for (; !error && it != end; it.increment(error))
    {
      const auto filename = it->path().filename().string();
      if (!validId(filename))
        continue;
      struct stat status{};
      if (::stat(it->path().c_str(), &status) != 0 || status.st_uid != ::getuid())
        continue;
      for (const auto &id :
           steamIdsFromEnvironment(readBounded(it->path() / "environ", 65536)))
      {
        auto &programs = result[id];
        auto add = [&programs](std::string name)
        {
          name = fcitx::stringutils::trim(name);
          if (!name.empty() && name.find(',') == std::string::npos &&
              std::none_of(name.begin(), name.end(),
                           fcitx::charutils::isspace))
            programs.insert(std::move(name));
        };
        add(readBounded(it->path() / "comm", 256));
        const auto command = readBounded(it->path() / "cmdline", 4096);
        add(fcitx::fs::baseName(command.substr(0, command.find('\0'))));
        if (proc == "/proc")
        {
          pid_t pid = 0;
          const auto [end, conversionError] = std::from_chars(
              filename.data(), filename.data() + filename.size(), pid);
          if (conversionError == std::errc{} &&
              end == filename.data() + filename.size())
            add(fcitx::getProcessName(pid));
        }
        else if (const auto executable =
                     fcitx::fs::readlink((it->path() / "exe").string()))
        {
          add(fcitx::fs::baseName(*executable));
        }
      }
    }
    return result;
  }

  std::set<std::string> splitGameList(std::string_view value)
  {
    std::string list(value);
    std::replace_if(
        list.begin(), list.end(),
        [](char character)
        { return character == ',' || fcitx::charutils::isspace(character); },
        ' ');
    std::set<std::string> result;
    const auto tokens = fcitx::stringutils::split(
        list, " ", fcitx::stringutils::SplitBehavior::SkipEmpty);
    result.insert(tokens.begin(), tokens.end());
    return result;
  }

  bool shouldExcludeGameInput(std::string_view program,
                              const std::set<std::string> &gamePrograms,
                              bool enabled, bool running, bool unknownFallback)
  {
    if (!enabled || !running)
      return false;
    if (!program.empty())
      return gamePrograms.count(std::string(program)) != 0;
    return unknownFallback;
  }

  std::string steamGameName(const std::string &id)
  {
    if (!validId(id))
      return {};
    std::set<std::filesystem::path> roots;
    const auto &data = fcitx::StandardPaths::global().userDirectory(
        fcitx::StandardPathsType::Data);
    if (!data.empty())
      roots.emplace(data / "Steam");
    if (const auto home = fcitx::getEnvironment("HOME"))
    {
      roots.emplace(std::filesystem::path(*home) / ".local/share/Steam");
      roots.emplace(std::filesystem::path(*home) / ".steam/steam");
      roots.emplace(std::filesystem::path(*home) /
                    ".var/app/com.valvesoftware.Steam/.local/share/Steam");
    }
    const std::regex pathPattern("\"path\"\\s*\"([^\"]+)\"");
    const auto initialRoots = roots;
    for (const auto &root : initialRoots)
    {
      const auto libraries =
          readBounded(root / "steamapps/libraryfolders.vdf", 1024 * 1024);
      for (auto it = std::sregex_iterator(libraries.begin(), libraries.end(),
                                          pathPattern);
           it != std::sregex_iterator(); ++it)
      {
        roots.emplace((*it)[1].str());
      }
    }
    const std::regex namePattern("\"name\"\\s*\"([^\"]+)\"");
    for (const auto &root : roots)
    {
      const auto manifest =
          readBounded(root / "steamapps" / ("appmanifest_" + id + ".acf"), 65536);
      std::smatch match;
      if (std::regex_search(manifest, match, namePattern))
        return match[1].str();
    }
    return {};
  }
} // namespace inputcounter
