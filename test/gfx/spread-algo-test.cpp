/****************************************************************
**spread-algo-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-12.
*
* Description: Unit tests for the spread-algo module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/spread-algo.hpp"

// config
#include "src/config/tile-enum.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::interval;
using ::gfx::point;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[spread] compute_icon_spread" ) {
  SpreadSpecs specs;
  Spreads expected;

  auto f = [&] { return compute_icon_spread( specs ); };

  specs = SpreadSpecs{
    .bounds = 209,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 1, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };

  expected = Spreads{
    .spreads = {
      { .spec           = { .count   = 14,
                            .trimmed = { .start = 6, .len = 9 } },
        .rendered_count = 14,
        .spacing        = 4 },
      { .spec           = { .count   = 6,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 6,
        .spacing        = 11 },
      { .spec           = { .count   = 0,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 0,
        .spacing        = 13 },
      { .spec           = { .count   = 1,
                            .trimmed = { .start = 4, .len = 8 } },
        .rendered_count = 1,
        .spacing        = 9 },
      { .spec           = { .count   = 15,
                            .trimmed = { .start = 2, .len = 12 } },
        .rendered_count = 15,
        .spacing        = 3 },
    } };

  REQUIRE( f() == expected );

  specs = SpreadSpecs{
    .bounds = 201,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 9, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 12, .trimmed = { .start = 3, .len = 14 } },
          { .count = 43, .trimmed = { .start = 2, .len = 16 } },
        },
    .group_spacing = 4 };

  expected = Spreads{
    .spreads = {
      { .spec           = { .count   = 14,
                            .trimmed = { .start = 6, .len = 9 } },
        .rendered_count = 14,
        .spacing        = 2 },
      { .spec           = { .count   = 9,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 9,
        .spacing        = 4 },
      { .spec           = { .count   = 0,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 0,
        .spacing        = 13 },
      { .spec           = { .count   = 12,
                            .trimmed = { .start = 3, .len = 14 } },
        .rendered_count = 12,
        .spacing        = 3 },
      { .spec           = { .count   = 43,
                            .trimmed = { .start = 2, .len = 16 } },
        .rendered_count = 43,
        .spacing        = 1 } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[spread] compute_icon_spread_OG" ) {
  SpreadSpecs specs;
  Spreads expected;

  auto f = [&] { return compute_icon_spread_OG( specs ); };

  specs = SpreadSpecs{
    .bounds = 209,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 1, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };

  expected = Spreads{
    .spreads = {
      { .spec           = { .count   = 14,
                            .trimmed = { .start = 6, .len = 9 } },
        .rendered_count = 14,
        .spacing        = 4 },
      { .spec           = { .count   = 6,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 6,
        .spacing        = 4 },
      { .spec           = { .count   = 0,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 0,
        .spacing        = 4 },
      { .spec           = { .count   = 1,
                            .trimmed = { .start = 4, .len = 8 } },
        .rendered_count = 1,
        .spacing        = 4 },
      { .spec           = { .count   = 15,
                            .trimmed = { .start = 2, .len = 12 } },
        .rendered_count = 15,
        .spacing        = 4 },
    } };

  REQUIRE( f() == expected );

  specs = SpreadSpecs{
    .bounds = 201,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 9, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 12, .trimmed = { .start = 3, .len = 14 } },
          { .count = 43, .trimmed = { .start = 2, .len = 16 } },
        },
    .group_spacing = 4 };

  expected = Spreads{
    .spreads = {
      { .spec           = { .count   = 14,
                            .trimmed = { .start = 6, .len = 9 } },
        .rendered_count = 14,
        .spacing        = 1 },
      { .spec           = { .count   = 9,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 9,
        .spacing        = 1 },
      { .spec           = { .count   = 0,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 0,
        .spacing        = 1 },
      { .spec           = { .count   = 12,
                            .trimmed = { .start = 3, .len = 14 } },
        .rendered_count = 12,
        .spacing        = 1 },
      { .spec           = { .count   = 43,
                            .trimmed = { .start = 2, .len = 16 } },
        .rendered_count = 43,
        .spacing        = 1 } } };
  REQUIRE( f() == expected );

  specs = SpreadSpecs{
    .bounds = 1000,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 1, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };

  expected = Spreads{
    .spreads = {
      { .spec           = { .count   = 14,
                            .trimmed = { .start = 6, .len = 9 } },
        .rendered_count = 14,
        .spacing        = 10 },
      { .spec           = { .count   = 6,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 6,
        .spacing        = 13 },
      { .spec           = { .count   = 0,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 0,
        .spacing        = 13 },
      { .spec           = { .count   = 1,
                            .trimmed = { .start = 4, .len = 8 } },
        .rendered_count = 1,
        .spacing        = 9 },
      { .spec           = { .count   = 15,
                            .trimmed = { .start = 2, .len = 12 } },
        .rendered_count = 15,
        .spacing        = 13 },
    } };

  REQUIRE( f() == expected );
}

TEST_CASE( "[spread] requires_label" ) {
}

TEST_CASE(
    "[spread] adjust_rendered_count_for_progress_count" ) {
}

} // namespace
} // namespace rn
