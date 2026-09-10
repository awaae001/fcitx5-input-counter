# Fcitx5 Input Counter

English | [简体中文](README.zh-CN.md)

An Fcitx5 addon that counts committed Unicode extended grapheme clusters and stores hourly totals in a local SQLite database.

The addon adds an `Input Counter` button to the Fcitx status area. Clicking it opens a Qt viewer window that shows totals (total / today / last 24 hours / last 7 days) and bar charts for the last 24 hours, 7 days, 30 days, 12 months, or all recorded years. Seven-day data uses six-hour bars. A custom chart accepts a start time, end time, and scale from one hour through one month. The viewer can also clear all recorded statistics. A Simplified Chinese translation is included.

## Scope

- Counts `InputContextCommitString` and `InputContextCommitStringWithCursor` events emitted by Fcitx input methods.
- Counts printable keys that Fcitx forwards directly to applications, including ordinary English input.
- Counts Unicode extended grapheme clusters as defined by UAX #29 within each committed event payload, including whitespace and punctuation. For example `👨‍👩‍👧‍👦` counts as one grapheme.
- Does not count control shortcuts or non-text keys.
- Persists hourly totals to `$XDG_DATA_HOME/fcitx5/input-counter/stats.db` (usually `~/.local/share/fcitx5/input-counter/stats.db`).
- Counts are buffered in memory and flushed to the database every 60 seconds, on shutdown, and whenever the statistics button is clicked.

## Game input filtering

Game filtering is enabled by default. It **only filters raw-key counting**;
`InputContextCommitString` and `InputContextCommitStringWithCursor` payloads
always count, including Chinese, English, punctuation, and emoji. It never
consumes or modifies application key events.

- A known game program is matched exactly against Fcitx's input-context program
  name. Switching to a recognized non-game application restores normal counting,
  even with a game running in the background.
- If the program name is empty, a matching Steam game process enables filtering
  as a fallback. This fallback can be disabled. An unknown **nonempty** program
  name is not automatically treated as a game.
- While filtering, a printable key's count is held until release. A second press
  before release, or Fcitx's repeat flag, discards that held key's count. Separate
  short taps still count, including game movement taps. Normal deliberate held
  keys in games are also excluded.
- Candidates contain only key identifiers, counts, and monotonic timestamps, not
  text. They are bounded to 64 held keys; a key's candidate count expires and is
  discarded after two seconds without events. Focus loss, context
  destruction/reset, input-method switches, and configuration changes discard
  candidates. Input paths without reliable releases can undercount;
  release/press-style autorepeat can escape filtering.

Settings are available in the Fcitx addon configuration, or in
`~/.config/fcitx5/conf/inputcounter.conf` (`$XDG_CONFIG_HOME` is respected):

To identify a program, focus its input field and inspect `focus:1` in:

```sh
busctl --user call org.fcitx.Fcitx5 /controller org.fcitx.Fcitx.Controller1 DebugInfo
```

## Build

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
```

Build dependencies: Fcitx5Core (>= 5.1.2), ICU, SQLite3, Qt6 DBus and Widgets,
Gettext.

## Install

### Arch Linux (AUR)

Install
[`fcitx5-input-counter`](https://aur.archlinux.org/packages/fcitx5-input-counter)
with an AUR helper, for example:

```sh
paru -S fcitx5-input-counter
```

### From source

The addon library must be installed in an Fcitx addon directory. A normal
system installation uses:

```sh
sudo cmake --install build
fcitx5 -r
```
