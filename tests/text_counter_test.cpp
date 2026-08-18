// SPDX-License-Identifier: MIT

//! Exercises raw-key classification and grapheme-cluster counting.

#include "text_counter.h"

#include <array>
#include <stdexcept>

#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>

namespace {

using inputcounter::TextCounter;

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void rejects_control_keys() {
  constexpr std::array controls{
      FcitxKey_BackSpace, FcitxKey_Tab,    FcitxKey_Linefeed,
      FcitxKey_Clear,     FcitxKey_Return, FcitxKey_Escape,
      FcitxKey_Delete,
  };

  for (const auto keySym : controls) {
    require(!TextCounter::textForKey(fcitx::Key(keySym)).has_value(),
            "control key was accepted as text");
  }
}

void accepts_printable_unicode() {
  const auto keySym = fcitx::Key::keySymFromUnicode(0x4e2d);
  const auto text = TextCounter::textForKey(fcitx::Key(keySym));

  require(text.has_value() && *text == "中",
          "printable Unicode key was not accepted");
}

void rejects_shortcut_chords() {
  const auto text = TextCounter::textForKey(
      fcitx::Key(FcitxKey_a, fcitx::KeyState::Ctrl));

  require(!text.has_value(), "shortcut chord was accepted as text");
}

void rejects_modifier_keys() {
  const auto text = TextCounter::textForKey(fcitx::Key(FcitxKey_Shift_L));

  require(!text.has_value(), "modifier key was accepted as text");
}

void counts_family_emoji_once() {
  TextCounter counter;

  const auto count = counter.count("👨‍👩‍👧‍👦");

  require(count == 1, "family emoji was split into code points");
}

void joins_combining_marks_to_their_base() {
  TextCounter counter;

  const auto count = counter.count("e\xcc\x81");

  require(count == 1, "combining mark was split from its base");
}

void counts_separate_graphemes_independently() {
  TextCounter counter;

  const auto count = counter.count("a👩‍💻中");

  require(count == 3, "separate graphemes were not counted independently");
}

} // namespace

int main() {
  rejects_control_keys();
  accepts_printable_unicode();
  rejects_shortcut_chords();
  rejects_modifier_keys();
  counts_family_emoji_once();
  joins_combining_marks_to_their_base();
  counts_separate_graphemes_independently();
}
