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
  TileSpreads tile_spreads;
  TileSpreadRenderPlan expected;

  auto f = [&] { return rendered_tile_spread( tile_spreads ); };

  // Test that a zero-count spread with a label requested does
  // not render a label.
  tile_spreads = {
    .spreads =
        {
          { .icon_spread = { .count   = 0,
                             .spacing = 1,
                             .width   = 1 },
            .tile        = e_tile::dragoon,
            .opaque      = interval{ .start = 2, .len = 28 },
            .label       = SpreadLabelOptions{} },
        },
    .group_spacing = 1,
  };
  expected = { .tiles = {}, .labels = {} };
  REQUIRE( f() == expected );

  // One spread, one tile, with label.
  tile_spreads = {
    .spreads =
        {
          { .icon_spread = { .count   = 1,
                             .spacing = 1,
                             .width   = 1 },
            .tile        = e_tile::dragoon,
            .opaque      = interval{ .start = 2, .len = 28 },
            .label       = SpreadLabelOptions{} },
        },
    .group_spacing = 1,
  };
  expected = {
    .tiles  = { pair{ e_tile::dragoon, point{ -2, 0 } } },
    .labels = {
      SpreadLabelRenderPlan{
        .text = "1",
        .p    = { .anchor    = { .x = 2, .y = 30 },
                  .placement = e_cdirection::sw } },
    } };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
