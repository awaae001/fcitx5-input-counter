# Fcitx5 Input Counter

An Fcitx5 addon that counts committed Unicode code points and stores hourly totals in a local SQLite database. It does not retain committed text.

The addon adds an `Input Counter` button to the Fcitx status area. Clicking it opens a Qt viewer window that shows totals (total / today / last 24 hours / last 7 days) and bar charts for the last 24 hours, 7 days, 30 days, 12 months,or all recorded years. Seven-day data uses six-hour bars. A custom chart acceptsa start time, end time, and scale from one hour through one month. The viewer can also clear all recorded statistics. A Simplified Chinese translation is included.

## Scope

- Counts `InputContextCommitString` and `InputContextCommitStringWithCursor`
  events emitted by Fcitx input methods.
- Counts printable keys that Fcitx forwards directly to applications, including
  ordinary English input.
- Counts Unicode code points, including whitespace and punctuation.
- Does not count control shortcuts or non-text keys.
- Persists hourly totals to
  `$XDG_DATA_HOME/fcitx5/input-counter/stats.db` (usually `~/.local/share/fcitx5/input-counter/stats.db`) with this schema:

- Counts are buffered in memory and flushed to the database every 60 seconds,
  on shutdown, and whenever the statistics button is clicked.

## Build

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
```

Build dependencies: Fcitx5Core (>= 5.1.2), SQLite3, Qt6 Widgets, Gettext.

## Install

The addon library must be installed in an Fcitx addon directory. A normal system installation uses:

```sh
sudo cmake --install build
fcitx5 -r
```

Use `DESTDIR` when building a distribution package instead of installing directly into the system.
