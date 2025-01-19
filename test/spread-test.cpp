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
          {
            .icon_spread  = { .real_count     = 0,
                              .rendered_count = 0,
                              .spacing        = 1,
                              .width          = 28 },
            .tile         = e_tile::dragoon,
            .opaque_start = 2,
          },
        },
    .group_spacing = 1,
    .label_policy  = SpreadLabel::always{},
  };
  expected = { .tiles = {}, .labels = {} };
  REQUIRE( f() == expected );

  // One spread, one tile, with label.
  tile_spreads = {
    .spreads =
        {
          {
            .icon_spread  = { .real_count     = 1,
                              .rendered_count = 1,
                              .spacing        = 1,
                              .width          = 28 },
            .tile         = e_tile::dragoon,
            .opaque_start = 2,
          },
        },
    .group_spacing = 1,
    .label_policy  = SpreadLabel::always{},
  };
  expected = {
    .tiles  = { pair{ e_tile::dragoon, point{ -2, 0 } } },
    .labels = {
      SpreadLabelRenderPlan{
        .text = "1",
        .p    = { .x = 2, .y = 32 - 8 - 2 - 2 },
      },
    } };
  REQUIRE( f() == expected );

  // Label uses real count when different from rendered_count.
  tile_spreads = {
    .spreads =
        {
          {
            .icon_spread  = { .real_count     = 2,
                              .rendered_count = 1,
                              .spacing        = 1,
                              .width          = 28 },
            .tile         = e_tile::dragoon,
            .opaque_start = 2,
          },
        },
    .group_spacing = 1,
    .label_policy  = SpreadLabel::always{},
  };
  expected = {
    .tiles  = { pair{ e_tile::dragoon, point{ -2, 0 } } },
    .labels = {
      SpreadLabelRenderPlan{
        .text = "2",
        .p    = { .x = 2, .y = 32 - 8 - 2 - 2 },
      },
    } };
  REQUIRE( f() == expected );

  // Three spreads, middle empty does not emit group spacing.
  tile_spreads = {
    .spreads =
        {
          {
            .icon_spread  = { .real_count     = 1,
                              .rendered_count = 1,
                              .spacing        = 1,
                              .width          = 28 },
            .tile         = e_tile::dragoon,
            .opaque_start = 2,
          },
          {
            .icon_spread  = { .real_count     = 0,
                              .rendered_count = 0,
                              .spacing        = 1,
                              .width          = 28 },
            .tile         = e_tile::dragoon,
            .opaque_start = 2,
          },
          {
            .icon_spread  = { .real_count     = 2,
                              .rendered_count = 2,
                              .spacing        = 3,
                              .width          = 13 },
            .tile         = e_tile::soldier,
            .opaque_start = 11,
          },
        },
    .group_spacing = 1,
    .label_policy  = SpreadLabel::always{},
  };
  expected = { .tiles =
                   {
                     pair{ e_tile::dragoon, point{ -2, 0 } },
                     pair{ e_tile::soldier, point{ 18, 0 } },
                     pair{ e_tile::soldier, point{ 21, 0 } },
                   },
               .labels = {
                 SpreadLabelRenderPlan{
                   .text = "1",
                   .p    = { .x = 2, .y = 32 - 8 - 2 - 2 },
                 },
                 SpreadLabelRenderPlan{
                   .text = "2",
                   .p    = { .x = 31, .y = 32 - 8 - 2 - 2 },
                 },
               } };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
