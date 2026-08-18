// SPDX-License-Identifier: MIT

//! Defines which raw keys are text and how committed text is measured.

#include "text_counter.h"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

#include <fcitx-utils/keysym.h>

#include <unicode/ubrk.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>

namespace inputcounter {

namespace {

constexpr bool isUnicodeControl(std::uint32_t codePoint) {
  return codePoint <= 0x1f || (codePoint >= 0x7f && codePoint <= 0x9f);
}

std::runtime_error icuError(const char *operation, UErrorCode status) {
  return std::runtime_error(std::string(operation) + ": " +
                            u_errorName(status));
}

} // namespace

TextCounter::TextCounter() {
  UErrorCode status = U_ZERO_ERROR;
  iterator_ = ubrk_open(UBRK_CHARACTER, "root", nullptr, 0, &status);
  if (U_FAILURE(status) || iterator_ == nullptr) {
    throw icuError("could not create ICU grapheme iterator", status);
  }
}

TextCounter::~TextCounter() { ubrk_close(iterator_); }

std::optional<std::string> TextCounter::textForKey(const fcitx::Key &key) {
  constexpr fcitx::KeyStates shortcutStates{
      fcitx::KeyState::Ctrl,  fcitx::KeyState::Alt,
      fcitx::KeyState::Super, fcitx::KeyState::Super2,
      fcitx::KeyState::Hyper, fcitx::KeyState::Hyper2,
      fcitx::KeyState::Meta};
  if (key.isModifier() || key.states().testAny(shortcutStates)) {
    return std::nullopt;
  }

  const auto codePoint = fcitx::Key::keySymToUnicode(key.sym());
  if (isUnicodeControl(codePoint)) {
    return std::nullopt;
  }

  auto text = fcitx::Key::keySymToUTF8(key.sym());
  if (text.empty()) {
    return std::nullopt;
  }
  return text;
}

std::size_t TextCounter::count(std::string_view text) {
  UErrorCode status = U_ZERO_ERROR;
  UText utf8Text = UTEXT_INITIALIZER;
  utext_openUTF8(&utf8Text, text.data(), static_cast<std::int64_t>(text.size()),
                 &status);
  if (U_FAILURE(status)) {
    utext_close(&utf8Text);
    throw icuError("could not open UTF-8 text", status);
  }

  ubrk_setUText(iterator_, &utf8Text, &status);
  utext_close(&utf8Text);
  if (U_FAILURE(status)) {
    throw icuError("could not set ICU grapheme text", status);
  }

  std::size_t result = 0;
  ubrk_first(iterator_);
  while (ubrk_next(iterator_) != UBRK_DONE) {
    ++result;
  }

  static constexpr UChar kEmptyText = 0;
  status = U_ZERO_ERROR;
  ubrk_setText(iterator_, &kEmptyText, 0, &status);
  if (U_FAILURE(status)) {
    throw icuError("could not release ICU grapheme text", status);
  }
  return result;
}

} // namespace inputcounter
