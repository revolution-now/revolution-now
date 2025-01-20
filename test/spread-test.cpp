/****************************************************************
**spread-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-12.
*
* Description: Unit tests for the spread module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/spread.hpp"

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

using ::base::nothing;
using ::gfx::interval;
using ::gfx::oriented_point;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::size;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[spread] compute_icon_spread" ) {
}

TEST_CASE( "[spread] requires_label" ) {
}

TEST_CASE( "[spread] rendered_tile_spread" ) {
  TileSpreadSpecs tile_spreads;
  TileSpread expected;

  auto f = [&] { return rendered_tile_spread( tile_spreads ); };

  // Test that a zero-count spread with a label requested does
  // not render a label.
  tile_spreads = {
    .spreads =
        {
          { .icon_spread = { .spec           = { .count   = 0,
                                                 .trimmed = { .start = 2,
                                                              .len =
                                                                  28 } },
                             .rendered_count = 0,
                             .spacing        = 1 },
            .tile        = e_tile::dragoon },
        },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{},
  };
  expected = { .tiles = {}, .labels = {} };
  REQUIRE( f() == expected );

  // One spread, one tile, with label.
  tile_spreads = {
    .spreads =
        {
          { .icon_spread = { .spec           = { .count   = 1,
                                                 .trimmed = { .start = 2,
                                                              .len =
                                                                  28 } },
                             .rendered_count = 1,
                             .spacing        = 1 },
            .tile        = e_tile::dragoon },
        },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{},
  };
  expected = {
    .tiles = { { .tile = e_tile::dragoon, .where = { -2, 0 } } },
    .labels = {
      SpreadLabelRenderPlan{
        .text  = "1",
        .where = { .x = 2, .y = 32 - 8 - 2 - 2 },
      },
    } };
  REQUIRE( f() == expected );

  // Label uses real count when different from rendered_count.
  tile_spreads = {
    .spreads =
        {
          { .icon_spread = { .spec           = { .count   = 2,
                                                 .trimmed = { .start = 2,
                                                              .len =
                                                                  28 } },
                             .rendered_count = 1,
                             .spacing        = 1 },
            .tile        = e_tile::dragoon },
        },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{},
  };
  expected = {
    .tiles = { { .tile = e_tile::dragoon, .where = { -2, 0 } } },
    .labels = {
      SpreadLabelRenderPlan{
        .text  = "2",
        .where = { .x = 2, .y = 32 - 8 - 2 - 2 },
      },
    } };
  REQUIRE( f() == expected );

  // Three spreads, middle empty does not emit group spacing.
  tile_spreads = {
    .spreads =
        {
          { .icon_spread = { .spec           = { .count   = 1,
                                                 .trimmed = { .start = 2,
                                                              .len =
                                                                  28 } },
                             .rendered_count = 1,
                             .spacing        = 1 },
            .tile        = e_tile::dragoon },
          { .icon_spread = { .spec           = { .count   = 0,
                                                 .trimmed = { .start = 2,
                                                              .len =
                                                                  28 } },
                             .rendered_count = 0,
                             .spacing        = 1 },
            .tile        = e_tile::dragoon },
          { .icon_spread = { .spec           = { .count   = 2,
                                                 .trimmed = { .start = 11,
                                                              .len =
                                                                  13 } },
                             .rendered_count = 2,
                             .spacing        = 3 },
            .tile        = e_tile::soldier },
        },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{},
  };
  expected = {
    .tiles =
        {
          { .tile = e_tile::dragoon, .where = { -2, 0 } },
          { .tile = e_tile::soldier, .where = { 18, 0 } },
          { .tile = e_tile::soldier, .where = { 21, 0 } },
        },
    .labels = {
      SpreadLabelRenderPlan{
        .text  = "1",
        .where = { .x = 2, .y = 32 - 8 - 2 - 2 },
      },
      SpreadLabelRenderPlan{
        .text  = "2",
        .where = { .x = 31, .y = 32 - 8 - 2 - 2 },
      },
    } };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
