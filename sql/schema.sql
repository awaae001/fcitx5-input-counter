-- Schema of the fcitx5 input counter statistics database.
-- hour is a Unix timestamp truncated to the start of its hour.
-- chars is the number of Unicode extended grapheme clusters counted in
-- committed event payloads during that hour, including whitespace and
-- punctuation. Control-key events are excluded.
CREATE TABLE IF NOT EXISTS stats(
  hour INTEGER PRIMARY KEY,
  chars INTEGER NOT NULL CHECK(chars >= 0)
);

PRAGMA user_version = 1;
