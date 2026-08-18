// SPDX-License-Identifier: MIT

#ifndef FCITX5_INPUT_COUNTER_TEXT_COUNTER_H
#define FCITX5_INPUT_COUNTER_TEXT_COUNTER_H

//! Defines which raw keys are text and how committed text is measured.

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include <fcitx-utils/key.h>

struct UBreakIterator;

namespace inputcounter {

/// Classifies direct keys and counts Unicode extended grapheme clusters.
///
/// One instance owns one mutable ICU iterator and must stay on its owning
/// thread.
class TextCounter final {
public:
  /// Creates a locale-independent Unicode character-boundary iterator.
  ///
  /// Throws `std::runtime_error` when ICU cannot create the iterator.
  TextCounter();

  /// Releases the owned ICU break iterator.
  ~TextCounter();

  TextCounter(const TextCounter &) = delete;
  TextCounter &operator=(const TextCounter &) = delete;

  /// Returns the UTF-8 text represented by an unhandled raw key.
  ///
  /// Modifier keys, shortcut chords, non-character keys, and Unicode C0/C1
  /// control characters do not represent committed text and return `nullopt`.
  static std::optional<std::string> textForKey(const fcitx::Key &key);

  /// Counts Unicode extended grapheme clusters in valid UTF-8 text.
  ///
  /// The caller must validate UTF-8 first. Throws `std::runtime_error` when
  /// ICU cannot attach the text to the iterator.
  std::size_t count(std::string_view text);

private:
  UBreakIterator *iterator_;
};

} // namespace inputcounter

#endif // FCITX5_INPUT_COUNTER_TEXT_COUNTER_H
