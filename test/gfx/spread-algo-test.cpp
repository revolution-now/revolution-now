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

  // Default case.
  specs    = {};
  expected = {};
  REQUIRE( f() == expected );

  // One empty spread.
  specs    = { .bounds        = 10,
               .specs         = { { .count   = 0,
                                    .trimmed = { .start = 6, .len = 9 } } },
               .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 0, .spacing = 1 } } };
  REQUIRE( f() == expected );

  // One spread with one count that doesn't fit.
  specs = {
    .bounds        = 10,
    .specs         = { { .count   = 1,
                         .trimmed = { .start = 6, .len = 11 } } },
    .group_spacing = 4 };
  REQUIRE( f() == nothing );

  // One spread with one count that perfectly fits.
  specs = {
    .bounds        = 10,
    .specs         = { { .count   = 1,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 1, .spacing = 1 } } };
  REQUIRE( f() == expected );

  // One spread with two count that doesn't fit.
  specs = {
    .bounds        = 10,
    .specs         = { { .count   = 2,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  REQUIRE( f() == nothing );

  // One spread with two count that perfectly fits with one spac-
  // ing.
  specs = {
    .bounds        = 11,
    .specs         = { { .count   = 2,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 2, .spacing = 1 } } };
  REQUIRE( f() == expected );

  // One spread with two count that fits with two spacing.
  specs = {
    .bounds        = 12,
    .specs         = { { .count   = 2,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 2, .spacing = 2 } } };
  REQUIRE( f() == expected );

  // One spread with two count that fits with 8 spacing.
  specs = {
    .bounds        = 18,
    .specs         = { { .count   = 2,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 2, .spacing = 8 } } };
  REQUIRE( f() == expected );

  // One spread with two count that fits with max spacing.
  specs = {
    .bounds        = 21,
    .specs         = { { .count   = 2,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 2, .spacing = 11 } } };
  REQUIRE( f() == expected );

  // One spread with two count that fits with capped max spacing.
  specs = {
    .bounds        = 30,
    .specs         = { { .count   = 2,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 2, .spacing = 11 } } };
  REQUIRE( f() == expected );

  // One spread with three count.
  specs = {
    .bounds        = 30,
    .specs         = { { .count   = 3,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 3, .spacing = 10 } } };
  REQUIRE( f() == expected );

  // One spread with three count with almost max spacing.
  specs = {
    .bounds        = 31,
    .specs         = { { .count   = 3,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 3, .spacing = 10 } } };
  REQUIRE( f() == expected );

  // One spread with three count with max spacing.
  specs = {
    .bounds        = 32,
    .specs         = { { .count   = 3,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 3, .spacing = 11 } } };
  REQUIRE( f() == expected );

  // One spread with three count with capped max spacing.
  specs = {
    .bounds        = 33,
    .specs         = { { .count   = 3,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 3, .spacing = 11 } } };
  REQUIRE( f() == expected );

  // One spread with 10 count.
  specs = {
    .bounds        = 33,
    .specs         = { { .count   = 10,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 10, .spacing = 2 } } };
  REQUIRE( f() == expected );

  // One spread with 10 count.
  specs = {
    .bounds        = 30,
    .specs         = { { .count   = 10,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 10, .spacing = 2 } } };
  REQUIRE( f() == expected );

  // One spread with 10 count.
  specs = {
    .bounds        = 29,
    .specs         = { { .count   = 10,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 10, .spacing = 2 } } };
  REQUIRE( f() == expected );

  // One spread with 10 count.
  specs = {
    .bounds        = 28,
    .specs         = { { .count   = 10,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 10, .spacing = 2 } } };
  REQUIRE( f() == expected );

  // One spread with 10 count.
  specs = {
    .bounds        = 27,
    .specs         = { { .count   = 10,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 10, .spacing = 1 } } };
  REQUIRE( f() == expected );

  // One spread with 10 count.
  specs = {
    .bounds        = 19,
    .specs         = { { .count   = 10,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  expected = Spreads{
    .spreads = { { .rendered_count = 10, .spacing = 1 } } };
  REQUIRE( f() == expected );

  // One spread with 10 count.
  specs = {
    .bounds        = 18,
    .specs         = { { .count   = 10,
                         .trimmed = { .start = 6, .len = 10 } } },
    .group_spacing = 4 };
  REQUIRE( f() == nothing );

  // Two spreads both empty.
  specs = {
    .bounds = 20,
    .specs =
        {
          { .count = 0, .trimmed = { .start = 6, .len = 9 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 0, .spacing = 1 },
                        { .rendered_count = 0, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads first empty.
  specs = {
    .bounds = 20,
    .specs =
        {
          { .count = 0, .trimmed = { .start = 6, .len = 9 } },
          { .count = 1, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 0, .spacing = 1 },
                        { .rendered_count = 1, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads second empty.
  specs = {
    .bounds = 20,
    .specs =
        {
          { .count = 1, .trimmed = { .start = 6, .len = 9 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 1, .spacing = 1 },
                        { .rendered_count = 0, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads don't fit.
  specs = {
    .bounds = 20,
    .specs =
        {
          { .count = 1, .trimmed = { .start = 6, .len = 9 } },
          { .count = 1, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  REQUIRE( f() == nothing );

  // Two spreads don't fit.
  specs = {
    .bounds = 24,
    .specs =
        {
          { .count = 1, .trimmed = { .start = 6, .len = 9 } },
          { .count = 1, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  REQUIRE( f() == nothing );

  // Two spreads perfectt fit.
  specs = {
    .bounds = 25,
    .specs =
        {
          { .count = 1, .trimmed = { .start = 6, .len = 9 } },
          { .count = 1, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 1, .spacing = 1 },
                        { .rendered_count = 1, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads five each, perfect fit.
  specs = {
    .bounds = 117,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 10 },
                        { .rendered_count = 5, .spacing = 13 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads five each, just below perfect fit.
  specs = {
    .bounds = 116,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 10 },
                        { .rendered_count = 5, .spacing = 12 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads five each, still below perfect fit.
  specs = {
    .bounds = 113,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 10 },
                        { .rendered_count = 5, .spacing = 12 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads five each, second spacing down again.
  specs = {
    .bounds = 112,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 10 },
                        { .rendered_count = 5, .spacing = 11 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads five each, no change.
  specs = {
    .bounds = 109,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 10 },
                        { .rendered_count = 5, .spacing = 11 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads five each, second spacing down again.
  specs = {
    .bounds = 108,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 10 },
                        { .rendered_count = 5, .spacing = 10 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads five each, no change.
  specs = {
    .bounds = 105,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 10 },
                        { .rendered_count = 5, .spacing = 10 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads five each, both down by one.
  specs = {
    .bounds = 104,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 9 },
                        { .rendered_count = 5, .spacing = 9 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads five each, no change.
  specs = {
    .bounds = 97,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 9 },
                        { .rendered_count = 5, .spacing = 9 },
                      } };
  REQUIRE( f() == expected );

  // Two spreads five each, both down.
  specs = {
    .bounds = 96,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 8 },
                        { .rendered_count = 5, .spacing = 8 },
                      } };
  REQUIRE( f() == expected );

  // Zero group spacing.
  specs = {
    .bounds = 96,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 0 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 9 },
                        { .rendered_count = 5, .spacing = 9 },
                      } };
  REQUIRE( f() == expected );

  // Zero group spacing, just before spacing is lowered.
  specs = {
    .bounds = 93,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 0 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 9 },
                        { .rendered_count = 5, .spacing = 9 },
                      } };
  REQUIRE( f() == expected );

  // Zero group spacing, spacing is lowered.
  specs = {
    .bounds = 92,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 0 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 8 },
                        { .rendered_count = 5, .spacing = 8 },
                      } };
  REQUIRE( f() == expected );

  // Zero group spacing super large.
  specs = {
    .bounds = 92,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 50 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 2 },
                        { .rendered_count = 5, .spacing = 2 },
                      } };
  REQUIRE( f() == expected );

  // Zero group spacing super large.
  specs = {
    .bounds = 87,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 50 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 2 },
                        { .rendered_count = 5, .spacing = 2 },
                      } };
  REQUIRE( f() == expected );

  // Zero group spacing super large.
  specs = {
    .bounds = 87,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 51 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 1 },
                        { .rendered_count = 5, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  // Zero group spacing super large.
  specs = {
    .bounds = 87,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 58 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 1 },
                        { .rendered_count = 5, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  // Zero group spacing super large, can't fit.
  specs = {
    .bounds = 87,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 9 } },
          { .count = 5, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 59 };
  REQUIRE( f() == nothing );

  // Zero trimmed len.
  specs = {
    .bounds = 96,
    .specs =
        {
          { .count = 5, .trimmed = { .start = 6, .len = 0 } },
          { .count = 5, .trimmed = { .start = 4, .len = 0 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 5, .spacing = 1 },
                        { .rendered_count = 5, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  specs = {
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
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 4 },
                        { .rendered_count = 6, .spacing = 4 },
                        { .rendered_count = 0, .spacing = 4 },
                        { .rendered_count = 1, .spacing = 4 },
                        { .rendered_count = 15, .spacing = 4 },
                      } };
  REQUIRE( f() == expected );

  specs = {
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
    .spreads = { { .rendered_count = 14, .spacing = 1 },
                 { .rendered_count = 9, .spacing = 1 },
                 { .rendered_count = 0, .spacing = 1 },
                 { .rendered_count = 12, .spacing = 1 },
                 { .rendered_count = 43, .spacing = 1 } } };
  REQUIRE( f() == expected );

  specs = {
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
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 10 },
                        { .rendered_count = 6, .spacing = 13 },
                        { .rendered_count = 0, .spacing = 13 },
                        { .rendered_count = 1, .spacing = 9 },
                        { .rendered_count = 15, .spacing = 13 },
                      } };
  REQUIRE( f() == expected );

  // One count super large.
  specs = {
    .bounds = 1000,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 200, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 4 },
                        { .rendered_count = 6, .spacing = 4 },
                        { .rendered_count = 0, .spacing = 4 },
                        { .rendered_count = 200, .spacing = 4 },
                        { .rendered_count = 15, .spacing = 4 },
                      } };
  REQUIRE( f() == expected );

  // One count super large, just below decrease.
  specs = {
    .bounds = 977,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 200, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 4 },
                        { .rendered_count = 6, .spacing = 4 },
                        { .rendered_count = 0, .spacing = 4 },
                        { .rendered_count = 200, .spacing = 4 },
                        { .rendered_count = 15, .spacing = 4 },
                      } };
  REQUIRE( f() == expected );

  // One count super large, spacing decreased.
  specs = {
    .bounds = 976,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 200, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 3 },
                        { .rendered_count = 6, .spacing = 3 },
                        { .rendered_count = 0, .spacing = 3 },
                        { .rendered_count = 200, .spacing = 3 },
                        { .rendered_count = 15, .spacing = 3 },
                      } };
  REQUIRE( f() == expected );

  // One count super large, just above decrease.
  specs = {
    .bounds = 746,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 200, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 3 },
                        { .rendered_count = 6, .spacing = 3 },
                        { .rendered_count = 0, .spacing = 3 },
                        { .rendered_count = 200, .spacing = 3 },
                        { .rendered_count = 15, .spacing = 3 },
                      } };
  REQUIRE( f() == expected );

  // One count super large, spacing decreased.
  specs = {
    .bounds = 745,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 200, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 2 },
                        { .rendered_count = 6, .spacing = 2 },
                        { .rendered_count = 0, .spacing = 2 },
                        { .rendered_count = 200, .spacing = 2 },
                        { .rendered_count = 15, .spacing = 2 },
                      } };
  REQUIRE( f() == expected );

  // One count super large, no change.
  specs = {
    .bounds = 515,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 200, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 2 },
                        { .rendered_count = 6, .spacing = 2 },
                        { .rendered_count = 0, .spacing = 2 },
                        { .rendered_count = 200, .spacing = 2 },
                        { .rendered_count = 15, .spacing = 2 },
                      } };
  REQUIRE( f() == expected );

  // One count super large, spacing decreased.
  specs = {
    .bounds = 514,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 200, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 1 },
                        { .rendered_count = 6, .spacing = 1 },
                        { .rendered_count = 0, .spacing = 1 },
                        { .rendered_count = 200, .spacing = 1 },
                        { .rendered_count = 15, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  // One count super large, no change.
  specs = {
    .bounds = 284,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 200, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 1 },
                        { .rendered_count = 6, .spacing = 1 },
                        { .rendered_count = 0, .spacing = 1 },
                        { .rendered_count = 200, .spacing = 1 },
                        { .rendered_count = 15, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  // One count super large, no more fit.
  specs = {
    .bounds = 283,
    .specs =
        {
          { .count = 14, .trimmed = { .start = 6, .len = 9 } },
          { .count = 6, .trimmed = { .start = 4, .len = 12 } },
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
          { .count = 200, .trimmed = { .start = 4, .len = 8 } },
          { .count = 15, .trimmed = { .start = 2, .len = 12 } },
        },
    .group_spacing = 4 };
  REQUIRE( f() == nothing );

  specs = {
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

  specs = {
    .bounds = 100,
    .specs =
        {
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 0, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  specs = {
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

  specs = {
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
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 1 },
                        { .rendered_count = 6, .spacing = 1 },
                        { .rendered_count = 0, .spacing = 1 },
                        { .rendered_count = 1, .spacing = 1 },
                        { .rendered_count = 15, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  specs = {
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
    .spreads = { { .rendered_count = 14, .spacing = 1 },
                 { .rendered_count = 9, .spacing = 1 },
                 { .rendered_count = 0, .spacing = 1 },
                 { .rendered_count = 12, .spacing = 1 },
                 { .rendered_count = 43, .spacing = 1 } } };
  REQUIRE( f() == expected );

  specs = {
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
  expected = Spreads{ .spreads = {
                        { .rendered_count = 14, .spacing = 1 },
                        { .rendered_count = 6, .spacing = 1 },
                        { .rendered_count = 0, .spacing = 1 },
                        { .rendered_count = 1, .spacing = 1 },
                        { .rendered_count = 15, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  specs = {
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
  expected = Spreads{ .spreads = {
                        { .rendered_count = 19, .spacing = 1 },
                        { .rendered_count = 8, .spacing = 1 },
                        { .rendered_count = 0, .spacing = 1 },
                        { .rendered_count = 1, .spacing = 1 },
                        { .rendered_count = 21, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  specs = {
    .bounds = 100,
    .specs =
        {
          { .count = 0, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 0, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );

  specs = {
    .bounds = 100,
    .specs =
        {
          { .count = 100, .trimmed = { .start = 4, .len = 12 } },
        },
    .group_spacing = 4 };
  expected = Spreads{ .spreads = {
                        { .rendered_count = 89, .spacing = 1 },
                      } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[spread] requires_label/Spread" ) {
  Spread spread;

  auto const f = [&] { return requires_label( spread ); };

  REQUIRE( f() == false );

  spread.spacing        = 1;
  spread.rendered_count = 1;
  REQUIRE( f() == false );

  spread.spacing        = 1;
  spread.rendered_count = 2;
  REQUIRE( f() == true );

  spread.spacing        = 2;
  spread.rendered_count = 2;
  REQUIRE( f() == false );

  spread.spacing        = 3;
  spread.rendered_count = 2;
  REQUIRE( f() == false );

  spread.rendered_count = 49;
  REQUIRE( f() == false );

  spread.rendered_count = 50;
  REQUIRE( f() == true );

  spread.rendered_count = 51;
  REQUIRE( f() == true );
}

TEST_CASE( "[spread] requires_label/ProgressSpread" ) {
  ProgressSpread spread;

  auto const f = [&] { return requires_label( spread ); };

  REQUIRE( f() == false );

  spread = { .spacings = {} };
  REQUIRE( f() == false );

  spread = { .spacings = {} };
  REQUIRE( f() == false );

  spread = { .spacings = { ProgressSpreadSpacing{} } };
  REQUIRE( f() == false );

  spread = { .spacings = { ProgressSpreadSpacing{
               .mod = 0, .spacing = 0 } } };
  REQUIRE( f() == false );

  spread = { .spacings = { ProgressSpreadSpacing{
               .mod = 1, .spacing = 0 } } };
  REQUIRE( f() == false );

  spread = { .spacings = { ProgressSpreadSpacing{
               .mod = 1, .spacing = 1 } } };
  REQUIRE( f() == true );

  spread = { .spacings = {
               ProgressSpreadSpacing{ .mod = 1, .spacing = 1 },
               ProgressSpreadSpacing{ .mod = 0, .spacing = 0 },
             } };
  REQUIRE( f() == true );
}

TEST_CASE(
    "[spread] adjust_rendered_count_for_progress_count "
    "(Spread)" ) {
  SpreadSpec spec;
  Spread spread;
  Spread expected;
  int progress_count = {};

  auto const f = [&]() -> auto const& {
    adjust_rendered_count_for_progress_count( spec, spread,
                                              progress_count );
    return spread;
  };

  // Default.
  spread         = {};
  progress_count = 0;
  expected       = {};
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=total_count.
  spec.count              = 1;
  spread.rendered_count   = 1;
  progress_count          = 1;
  expected                = spread;
  expected.rendered_count = 1;
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=0.
  spec.count              = 1;
  spread.rendered_count   = 1;
  progress_count          = 0;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=total_count.
  spec.count              = 100;
  spread.rendered_count   = 100;
  progress_count          = 100;
  expected                = spread;
  expected.rendered_count = 100;
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=0.
  spec.count              = 100;
  spread.rendered_count   = 100;
  progress_count          = 0;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=30.
  spec.count              = 100;
  spread.rendered_count   = 100;
  progress_count          = 30;
  expected                = spread;
  expected.rendered_count = 30;
  REQUIRE( f() == expected );

  // rendered_count=total count, progress_count=-30.
  spec.count              = 100;
  spread.rendered_count   = 100;
  progress_count          = -30;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=total_count.
  spec.count              = 1;
  spread.rendered_count   = 0;
  progress_count          = 1;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=0.
  spec.count              = 1;
  spread.rendered_count   = 0;
  progress_count          = 0;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=total_count.
  spec.count              = 100;
  spread.rendered_count   = 50;
  progress_count          = 100;
  expected                = spread;
  expected.rendered_count = 50;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=0.
  spec.count              = 100;
  spread.rendered_count   = 50;
  progress_count          = 0;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=30.
  spec.count              = 100;
  spread.rendered_count   = 50;
  progress_count          = 30;
  expected                = spread;
  expected.rendered_count = 15;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=-30.
  spec.count              = 100;
  spread.rendered_count   = 50;
  progress_count          = -30;
  expected                = spread;
  expected.rendered_count = 0;
  REQUIRE( f() == expected );

  // rendered_count<total count, progress_count=1.
  // NOTE: this one tests that rendered_count is lifted to 1 in
  // the case that rendered count is left as zero after the frac-
  // tional calculation but progress_count is non-zero.
  spec.count              = 1000;
  spread.rendered_count   = 50;
  progress_count          = 1;
  expected                = spread;
  expected.rendered_count = 1;
  REQUIRE( f() == expected );

  // Test progress count larger than total count.
  spec.count              = 100;
  spread.rendered_count   = 100;
  progress_count          = 200;
  expected                = spread;
  expected.rendered_count = 100;
  REQUIRE( f() == expected );

  // Test progress count larger than total count with
  // rendered_count < total_count.
  spec.count              = 100;
  spread.rendered_count   = 50;
  progress_count          = 200;
  expected                = spread;
  expected.rendered_count = 50;
  REQUIRE( f() == expected );

  // Test rendered_count > total_count.
  spec.count              = 100;
  spread.rendered_count   = 200;
  progress_count          = 100;
  expected                = spread;
  expected.rendered_count = 100;
  REQUIRE( f() == expected );

  // Test rendered_count > total_count with progress_count <
  // total_count.
  spec.count              = 100;
  spread.rendered_count   = 200;
  progress_count          = 50;
  expected                = spread;
  expected.rendered_count = 50;
  REQUIRE( f() == expected );
}

TEST_CASE( "[spread] compute_icon_spread_progress_bar" ) {
  ProgressSpreadSpec spec;
  ProgressSpread expected;
  interval trimmed;

  auto f = [&] {
    return compute_icon_spread_progress_bar( spec );
  };

  // Default case.
  spec     = {};
  expected = {};
  REQUIRE( f() == expected );

  // ============================================================
  trimmed = { .start = 0, .len = 0 };
  // ============================================================

  // Zero bounds with non-zero count.
  spec     = { .bounds      = 0,
               .spread_spec = { .count = 1, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 1,
               .spread_spec = { .count = 2, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 4,
               .spread_spec = { .count = 5, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  // ============================================================
  trimmed = { .start = 2, .len = 5 };
  // ============================================================

  // Zero bounds with non-zero count.
  spec = { .bounds      = 0,
           .spread_spec = { .count = 1, .trimmed = trimmed } };
  REQUIRE( f() == nothing );

  // Not quite enough space.
  spec = { .bounds      = 4,
           .spread_spec = { .count = 1, .trimmed = trimmed } };
  REQUIRE( f() == nothing );

  // Just enough space.
  spec     = { .bounds      = 5,
               .spread_spec = { .count = 1, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 6 },
               } };
  REQUIRE( f() == expected );

  // Ramp down of count.

  spec = { .bounds      = 30,
           .spread_spec = { .count = 30, .trimmed = trimmed } };
  REQUIRE( f() == nothing );

  spec = { .bounds      = 30,
           .spread_spec = { .count = 29, .trimmed = trimmed } };
  REQUIRE( f() == nothing );

  spec = { .bounds      = 30,
           .spread_spec = { .count = 28, .trimmed = trimmed } };
  REQUIRE( f() == nothing );

  spec = { .bounds      = 30,
           .spread_spec = { .count = 27, .trimmed = trimmed } };
  REQUIRE( f() == nothing );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 26, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 25, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 13, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 24, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 8, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 23, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 6, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 22, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 5, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 21, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 4, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 20, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 3, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 19, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 3, .spacing = 1 },
                 { .mod = 10, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 18, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 2, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 17, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 2, .spacing = 1 },
                 { .mod = 9, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 16, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 2, .spacing = 1 },
                 { .mod = 4, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 15, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 2, .spacing = 1 },
                 { .mod = 3, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 14, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 2, .spacing = 2 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 13, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 2 },
                 { .mod = 7, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 12, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 2 },
                 { .mod = 3, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 11, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 2 },
                 { .mod = 2, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 10, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 2 },
                 { .mod = 2, .spacing = 1 },
                 { .mod = 3, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 9, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 3 },
                 { .mod = 5, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 8, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 3 },
                 { .mod = 2, .spacing = 1 },
                 { .mod = 4, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 7, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 4 },
                 { .mod = 4, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 6, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 5 },
               } };
  REQUIRE( f() == expected );

  // Hits max spacing.
  spec     = { .bounds      = 30,
               .spread_spec = { .count = 5, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 6 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 4, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 6 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 3, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 6 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 2, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 6 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 1, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 6 },
               } };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = 0, .trimmed = trimmed } };
  expected = { .spacings = {} };
  REQUIRE( f() == expected );

  spec     = { .bounds      = 30,
               .spread_spec = { .count = -1, .trimmed = trimmed } };
  expected = { .spacings = {} };
  REQUIRE( f() == expected );

  // ============================================================
  trimmed = { .start = 2, .len = 16 };
  // ============================================================

  // count=0
  spec     = { .bounds      = 626,
               .spread_spec = { .count = 0, .trimmed = trimmed } };
  expected = { .spacings = {} };
  REQUIRE( f() == expected );

  // count=1
  spec     = { .bounds      = 626,
               .spread_spec = { .count = 1, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 17 },
               } };
  REQUIRE( f() == expected );

  // count=1
  spec     = { .bounds      = 626,
               .spread_spec = { .count = 2, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 17 },
               } };
  REQUIRE( f() == expected );

  // count=35
  spec     = { .bounds      = 626,
               .spread_spec = { .count = 35, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 17 },
               } };
  REQUIRE( f() == expected );

  // count=36
  spec     = { .bounds      = 626,
               .spread_spec = { .count = 36, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 17 },
               } };
  REQUIRE( f() == expected );

  // count=37
  spec     = { .bounds      = 626,
               .spread_spec = { .count = 37, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 16 },
                 { .mod = 2, .spacing = 1 },
                 { .mod = 3, .spacing = 1 },
                 { .mod = 8, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  // count=161.
  spec     = { .bounds      = 626,
               .spread_spec = { .count = 161, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 3 },
                 { .mod = 2, .spacing = 1 },
                 { .mod = 4, .spacing = 1 },
                 { .mod = 15, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  // count=611.
  spec     = { .bounds      = 626,
               .spread_spec = { .count = 611, .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  // count=612.
  spec = { .bounds      = 626,
           .spread_spec = { .count = 612, .trimmed = trimmed } };
  REQUIRE( f() == nothing );

  // count=int_max.
  spec = { .bounds      = 626,
           .spread_spec = { .count = numeric_limits<int>::max(),
                            .trimmed = trimmed } };
  REQUIRE( f() == nothing );

  // bounds=int_max, count=int_max.
  spec = { .bounds      = numeric_limits<int>::max(),
           .spread_spec = { .count = numeric_limits<int>::max(),
                            .trimmed = trimmed } };
  REQUIRE( f() == nothing );

  // bounds=int_max, count=int_max-15.
  spec = {
    .bounds      = numeric_limits<int>::max(),
    .spread_spec = { .count   = numeric_limits<int>::max() - 15,
                     .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
               } };
  REQUIRE( f() == expected );

  // bounds=int_max, count=int_max-16.
  spec = {
    .bounds      = numeric_limits<int>::max(),
    .spread_spec = { .count   = numeric_limits<int>::max() - 16,
                     .trimmed = trimmed } };
  expected = { .spacings = {
                 { .mod = 1, .spacing = 1 },
                 { .mod = 1073741816, .spacing = 1 },
               } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[spread] compute_icon_spread_inhomogeneous" ) {
  InhomogeneousSpreadSpec spec;
  InhomogeneousSpread expected;

  auto f = [&] {
    return compute_icon_spread_inhomogeneous( spec );
  };

  auto constexpr kMax = numeric_limits<int>::max();

  // Default case.
  spec     = {};
  expected = { .max_total_spacing = kMax };
  REQUIRE( f() == expected );

  // Single element, doesn't fit.
  spec = { .bounds = 10, .max_spacing = 1, .widths = { 11 } };
  REQUIRE( f() == nothing );

  // Single element, just fits.
  spec = { .bounds = 10, .max_spacing = 1, .widths = { 10 } };
  expected = { .max_total_spacing = kMax };
  REQUIRE( f() == expected );

  // Single element, fits with space left.
  spec     = { .bounds = 10, .max_spacing = 1, .widths = { 9 } };
  expected = { .max_total_spacing = kMax };
  REQUIRE( f() == expected );

  // Single element, fits with space left.
  spec     = { .bounds = 10, .max_spacing = 1, .widths = { 4 } };
  expected = { .max_total_spacing = kMax };
  REQUIRE( f() == expected );

  // Single element, fits with space left.
  spec     = { .bounds = 10, .max_spacing = 1, .widths = { 0 } };
  expected = { .max_total_spacing = kMax };
  REQUIRE( f() == expected );

  // Two elements, don't fit.
  spec = { .bounds = 10, .max_spacing = 1, .widths = { 1, 11 } };
  REQUIRE( f() == nothing );

  // Two elements, just fits.
  spec = { .bounds = 10, .max_spacing = 1, .widths = { 1, 10 } };
  expected = { .max_total_spacing = 0 };
  REQUIRE( f() == expected );

  spec = { .bounds = 10, .max_spacing = 1, .widths = { 1, 9 } };
  expected = { .max_total_spacing = 1 };
  REQUIRE( f() == expected );

  spec = { .bounds = 10, .max_spacing = 1, .widths = { 1, 8 } };
  expected = { .max_total_spacing = kMax };
  REQUIRE( f() == expected );

  // Adjust max spacing.
  spec = { .bounds = 10, .max_spacing = 2, .widths = { 1, 8 } };
  expected = { .max_total_spacing = 2 };
  REQUIRE( f() == expected );

  spec = { .bounds = 10, .max_spacing = 2, .widths = { 1, 7 } };
  expected = { .max_total_spacing = kMax };
  REQUIRE( f() == expected );

  spec = { .bounds = 10, .max_spacing = 2, .widths = { 1, 3 } };
  expected = { .max_total_spacing = kMax };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 10, .max_spacing = 2, .widths = { 5, 1, 3 } };
  expected = { .max_total_spacing = 4 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 10, .max_spacing = 3, .widths = { 5, 1, 3 } };
  expected = { .max_total_spacing = 3 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 10, .max_spacing = 4, .widths = { 5, 1, 3 } };
  expected = { .max_total_spacing = 3 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 10, .max_spacing = 5, .widths = { 5, 1, 3 } };
  expected = { .max_total_spacing = 3 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 10, .max_spacing = 2, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = 2 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 10, .max_spacing = 3, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = 2 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 10, .max_spacing = 1, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = 3 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 11, .max_spacing = 1, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = 3 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 12, .max_spacing = 1, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = 4 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 12, .max_spacing = 0, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = kMax };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 11, .max_spacing = 0, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = kMax };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 10, .max_spacing = 0, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = 4 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 9, .max_spacing = 0, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = 3 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 8, .max_spacing = 0, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = 2 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 7, .max_spacing = 0, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = 2 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 6, .max_spacing = 0, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = 1 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 5, .max_spacing = 0, .widths = { 5, 1, 3, 2 } };
  expected = { .max_total_spacing = 1 };
  REQUIRE( f() == expected );

  spec = {
    .bounds = 4, .max_spacing = 0, .widths = { 5, 1, 3, 2 } };
  REQUIRE( f() == nothing );

  spec = {
    .bounds = 0, .max_spacing = 0, .widths = { 5, 1, 3, 2 } };
  REQUIRE( f() == nothing );
}

} // namespace
} // namespace rn
