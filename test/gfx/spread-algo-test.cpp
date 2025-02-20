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

  specs    = {};
  expected = {};
  REQUIRE( f() == expected );

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

  specs = SpreadSpecs{
    .bounds = 100,
    .specs =
        {
          { .count = 140, .trimmed = { .start = 6, .len = 9 } },
          { .count = 60, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 10, .trimmed = { .start = 4, .len = 8 } },
          { .count = 150, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };
  REQUIRE( f() == nothing );

  specs = SpreadSpecs{
    .bounds = 100,
    .specs =
        {
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = {
      { .spec           = { .count   = 0,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 0,
        .spacing        = 1 },
    } };
  REQUIRE( f() == expected );

  specs = SpreadSpecs{
    .bounds = 100,
    .specs =
        {
          { .count = 100, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  REQUIRE( f() == nothing );
}

TEST_CASE( "[spread] compute_icon_spread_proportionate" ) {
  SpreadSpecs specs;
  Spreads expected;

  auto f = [&] {
    return compute_icon_spread_proportionate( specs );
  };

  specs    = {};
  expected = {};
  REQUIRE( f() == expected );

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
        .spacing        = 1 },
      { .spec           = { .count   = 6,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 6,
        .spacing        = 1 },
      { .spec           = { .count   = 0,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 0,
        .spacing        = 1 },
      { .spec           = { .count   = 1,
                            .trimmed = { .start = 4, .len = 8 } },
        .rendered_count = 1,
        .spacing        = 1 },
      { .spec           = { .count   = 15,
                            .trimmed = { .start = 2, .len = 12 } },
        .rendered_count = 15,
        .spacing        = 1 },
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
        .spacing        = 1 },
      { .spec           = { .count   = 6,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 6,
        .spacing        = 1 },
      { .spec           = { .count   = 0,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 0,
        .spacing        = 1 },
      { .spec           = { .count   = 1,
                            .trimmed = { .start = 4, .len = 8 } },
        .rendered_count = 1,
        .spacing        = 1 },
      { .spec           = { .count   = 15,
                            .trimmed = { .start = 2, .len = 12 } },
        .rendered_count = 15,
        .spacing        = 1 },
    } };
  REQUIRE( f() == expected );

  specs = SpreadSpecs{
    .bounds = 100,
    .specs =
        {
          { .count = 140, .trimmed = { .start = 6, .len = 9 } },
          { .count = 60, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 10, .trimmed = { .start = 4, .len = 8 } },
          { .count = 150, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = {
      { .spec           = { .count   = 140,
                            .trimmed = { .start = 6, .len = 9 } },
        .rendered_count = 19,
        .spacing        = 1 },
      { .spec           = { .count   = 60,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 8,
        .spacing        = 1 },
      { .spec           = { .count   = 0,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 0,
        .spacing        = 1 },
      { .spec           = { .count   = 10,
                            .trimmed = { .start = 4, .len = 8 } },
        .rendered_count = 1,
        .spacing        = 1 },
      { .spec           = { .count   = 150,
                            .trimmed = { .start = 2, .len = 12 } },
        .rendered_count = 21,
        .spacing        = 1 },
    } };
  REQUIRE( f() == expected );

  specs = SpreadSpecs{
    .bounds = 100,
    .specs =
        {
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = {
      { .spec           = { .count   = 0,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 0,
        .spacing        = 1 },
    } };
  REQUIRE( f() == expected );

  specs = SpreadSpecs{
    .bounds = 100,
    .specs =
        {
          { .count = 100, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = {
      { .spec           = { .count   = 100,
                            .trimmed = { .start = 4, .len = 12 } },
        .rendered_count = 89,
        .spacing        = 1 },
    } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[spread] requires_label" ) {
  Spread spread;

  auto const f = [&] { return requires_label( spread ); };

  REQUIRE( f() == true );

  spread.spacing = 1;
  REQUIRE( f() == true );

  spread.spacing = 2;
  REQUIRE( f() == false );

  spread.spacing = 3;
  REQUIRE( f() == false );

  spread.rendered_count = 49;
  REQUIRE( f() == false );

  spread.rendered_count = 50;
  REQUIRE( f() == true );

  spread.rendered_count = 51;
  REQUIRE( f() == true );
}

TEST_CASE(
    "[spread] adjust_rendered_count_for_progress_count" ) {
  Spread spread;
  Spread expected;
  int progress_count = {};

  auto const f = [&]() -> auto const& {
    adjust_rendered_count_for_progress_count( spread,
                                              progress_count );
    return spread;
  };

  // Default.
  spread         = {};
  progress_count = 0;
  expected       = {};
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=total_count.
  spread.spec.count       = 1;
  spread.rendered_count   = 1;
  progress_count          = 1;
  expected                = spread;
  expected.rendered_count = 1;
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=0.
  spread.spec.count       = 1;
  spread.rendered_count   = 1;
  progress_count          = 0;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=total_count.
  spread.spec.count       = 100;
  spread.rendered_count   = 100;
  progress_count          = 100;
  expected                = spread;
  expected.rendered_count = 100;
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=0.
  spread.spec.count       = 100;
  spread.rendered_count   = 100;
  progress_count          = 0;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=30.
  spread.spec.count       = 100;
  spread.rendered_count   = 100;
  progress_count          = 30;
  expected                = spread;
  expected.rendered_count = 30;
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=-30.
  spread.spec.count       = 100;
  spread.rendered_count   = 100;
  progress_count          = -30;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=total_count.
  spread.spec.count       = 1;
  spread.rendered_count   = 0;
  progress_count          = 1;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=0.
  spread.spec.count       = 1;
  spread.rendered_count   = 0;
  progress_count          = 0;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=total_count.
  spread.spec.count       = 100;
  spread.rendered_count   = 50;
  progress_count          = 100;
  expected                = spread;
  expected.rendered_count = 50;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=0.
  spread.spec.count       = 100;
  spread.rendered_count   = 50;
  progress_count          = 0;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=30.
  spread.spec.count       = 100;
  spread.rendered_count   = 50;
  progress_count          = 30;
  expected                = spread;
  expected.rendered_count = 15;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=-30.
  spread.spec.count       = 100;
  spread.rendered_count   = 50;
  progress_count          = -30;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=1.
  // NOTE: this one tests that rendered_count is lifted to 1 in
  // the case that rendered count is left as zero after the frac-
  // tional calculation but progress_count is non-zero.
  spread.spec.count       = 1000;
  spread.rendered_count   = 50;
  progress_count          = 1;
  expected                = spread;
  expected.rendered_count = 1;
  REQUIRE( f() == expected );

  // Test progress count larger than total count.
  spread.spec.count       = 100;
  spread.rendered_count   = 100;
  progress_count          = 200;
  expected                = spread;
  expected.rendered_count = 100;
  REQUIRE( f() == expected );

  // Test progress count larger than total count with
  // rendered_count < total_count.
  spread.spec.count       = 100;
  spread.rendered_count   = 50;
  progress_count          = 200;
  expected                = spread;
  expected.rendered_count = 50;
  REQUIRE( f() == expected );

  // Test rendered_count > total_count.
  spread.spec.count       = 100;
  spread.rendered_count   = 200;
  progress_count          = 100;
  expected                = spread;
  expected.rendered_count = 100;
  REQUIRE( f() == expected );

  // Test rendered_count > total_count with progress_count <
  // total_count.
  spread.spec.count       = 100;
  spread.rendered_count   = 200;
  progress_count          = 50;
  expected                = spread;
  expected.rendered_count = 50;
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
