# Fcitx5 Input Counter

An Fcitx5 addon that counts committed Unicode code points and stores hourly
totals in a local SQLite database. It does not retain committed text.

The addon adds an `Input statistics` button to the Fcitx status area. Clicking
it opens a Qt viewer window that shows totals (total / today / last 24 hours /
last 7 days) and a bar chart of the last 24 hours, 7 days, or 30 days. The
viewer can also clear all recorded statistics. A Simplified Chinese
translation is included.

## Scope

- Counts `InputContextCommitString` and `InputContextCommitStringWithCursor`
  events emitted by Fcitx input methods.
- Counts printable keys that Fcitx forwards directly to applications, including
  ordinary English input.
- Counts Unicode code points, including whitespace and punctuation.
- Does not count control shortcuts or non-text keys.
- Persists hourly totals to
  `$XDG_DATA_HOME/fcitx5/input-counter/stats.db` (usually
  `~/.local/share/fcitx5/input-counter/stats.db`) with this schema:

  ```sql
  CREATE TABLE stats(
    hour INTEGER PRIMARY KEY,              -- Unix timestamp truncated to the hour
    chars INTEGER NOT NULL CHECK(chars >= 0)
  );
  ```

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

The addon library must be installed in an Fcitx addon directory. A normal
system installation uses:

```sh
sudo cmake --install build
fcitx5 -r
```

The installation places:

```text
/usr/lib/fcitx5/inputcounter.so
/usr/bin/fcitx5-input-counter-viewer
/usr/share/fcitx5/addon/inputcounter.conf
/usr/share/locale/zh_CN/LC_MESSAGES/fcitx5-input-counter.mo
```

Use `DESTDIR` when building a distribution package instead of installing
directly into the system.
