// SPDX-License-Identifier: MIT
#ifndef INPUT_COUNTER_GAME_KEY_FILTER_H
#define INPUT_COUNTER_GAME_KEY_FILTER_H

#include <cstdint>
#include <unordered_map>

namespace inputcounter
{

  /// Buffers counts, never text. Call only while game filtering is active.
  /// Missing releases expire by discarding candidates, not by assuming a commit.
  class GameKeyFilter
  {
  public:
    static constexpr std::uint64_t timeoutUsec = 2'000'000;

    void press(std::uint64_t key, std::uint64_t chars, bool repeat,
               std::uint64_t now)
    {
      expire(now);
      auto it = pending_.find(key);
      if (it != pending_.end())
      {
        it->second.chars = 0;
        it->second.last = now;
      }
      else if (pending_.size() < 64)
      {
        pending_.emplace(key, Pending{repeat ? 0 : chars, now});
      }
    }
    std::uint64_t release(std::uint64_t key, std::uint64_t now)
    {
      expire(now);
      const auto it = pending_.find(key);
      if (it == pending_.end())
      {
        return 0;
      }
      const auto chars = it->second.chars;
      pending_.erase(it);
      return chars;
    }

    void expire(std::uint64_t now)
    {
      for (auto it = pending_.begin(); it != pending_.end();)
      {
        if (now - it->second.last >= timeoutUsec)
        {
          it = pending_.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
    
    void clear() { pending_.clear(); }

  private:
    struct Pending
    {
      std::uint64_t chars;
      std::uint64_t last;
    };
    std::unordered_map<std::uint64_t, Pending> pending_;
  };

} // namespace inputcounter
#endif
