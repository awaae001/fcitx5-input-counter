// SPDX-License-Identifier: MIT

//! Exercises viewer-side query construction and response formatting.

#include "statistics_snapshot.h"

#include <stdexcept>
#include <vector>

namespace {

using inputcounter::allTimeQuery;
using inputcounter::ChartQuery;
using inputcounter::last24HoursQuery;
using inputcounter::makeBars;
using inputcounter::ranges;

void require(bool condition, const char *message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void builds_twenty_four_contiguous_hour_buckets() {
  const auto query = last24HoursQuery(100 * 3600 + 123);

  require(query.size() == 24, "hour query did not contain 24 buckets");
  for (std::size_t index = 0; index < query.size(); ++index) {
    require(query[index].range.end - query[index].range.start == 3600,
            "hour bucket had the wrong duration");
    if (index > 0) {
      require(query[index - 1].range.end == query[index].range.start,
              "hour buckets were not contiguous");
    }
  }
}

void builds_one_bucket_per_calendar_year() {
  const auto query = allTimeQuery(0, 2 * 365 * 24 * 3600);

  require(query.size() == 3, "year query had the wrong bucket count");
  require(query.front().label == QStringLiteral("1970"),
          "first year label was incorrect");
  require(query.back().label == QStringLiteral("1972"),
          "last year label was incorrect");
}

void extracts_wire_ranges() {
  const ChartQuery query{{{0, 10}, QStringLiteral("a")},
                         {{10, 20}, QStringLiteral("b")}};

  const auto result = ranges(query);

  require(result.size() == 2 && result[0].start == 0 && result[1].end == 20,
          "wire ranges did not preserve query boundaries");
}

void pairs_counts_with_labels() {
  const ChartQuery query{{{0, 10}, QStringLiteral("a")},
                         {{10, 20}, QStringLiteral("b")}};

  const auto bars = makeBars(query, {3, 5});

  require(bars.size() == 2 && bars[0].first == QStringLiteral("a") &&
              bars[0].second == 3 && bars[1].second == 5,
          "bar values did not match query labels");
}

void rejects_mismatched_bucket_counts() {
  const ChartQuery query{{{0, 10}, QStringLiteral("a")}};
  bool rejected = false;
  try {
    makeBars(query, {});
  } catch (const std::invalid_argument &) {
    rejected = true;
  }

  require(rejected, "mismatched bucket values were accepted");
}

} // namespace

int main() {
  builds_twenty_four_contiguous_hour_buckets();
  builds_one_bucket_per_calendar_year();
  extracts_wire_ranges();
  pairs_counts_with_labels();
  rejects_mismatched_bucket_counts();
}
