# Fcitx5 Input Counter

An Fcitx5 addon that counts committed Unicode code points in memory. It does
not retain committed text and resets whenever Fcitx exits.

The current count appears in the Fcitx status menu as `Session input: N
characters`. A Simplified Chinese translation is included.

## Scope

- Counts `InputContextCommitString` and `InputContextCommitStringWithCursor`
  events emitted by Fcitx input methods.
- Counts printable keys that Fcitx forwards directly to applications, including
  ordinary English input.
- Counts Unicode code points, including whitespace and punctuation.
- Does not count control shortcuts or non-text keys.
- Does not persist data.

## Build

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build
```

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
/usr/share/fcitx5/addon/inputcounter.conf
/usr/share/locale/zh_CN/LC_MESSAGES/fcitx5-input-counter.mo
```

Use `DESTDIR` when building a distribution package instead of installing
directly into the system.
