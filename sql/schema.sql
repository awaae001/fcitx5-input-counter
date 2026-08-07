-- Schema of the fcitx5 input counter statistics database.
-- hour is a Unix timestamp truncated to the start of its hour.
CREATE TABLE IF NOT EXISTS stats(
  hour INTEGER PRIMARY KEY,
  chars INTEGER NOT NULL CHECK(chars >= 0)
);

PRAGMA user_version = 1;
