/****************************************************************
**spread-builder-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-20.
*
* Description: Unit tests for the spread-builder module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/spread-builder.hpp"

// Testing
#include "test/mocking.hpp"
#include "test/mocks/render/itextometer.hpp"

// Revolution Now
#include "src/tiles.hpp"

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
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[spread-builder] build_fixed_tile_spread" ) {
  FixedTileSpreadConfig config;
  TileSpreadRenderPlan ex;

  auto f = [&] { return build_fixed_tile_spread( config ); };

  testing_set_trimmed_cache(
      e_tile::commodity_tobacco_20,
      rect{ .origin = { .x = 4, .y = 1 },
            .size   = { .w = 13, .h = 19 } } );

  config = { .tile = e_tile::commodity_tobacco_20 };
  REQUIRE( f() == ex );

  config = { .tile           = e_tile::commodity_tobacco_20,
             .rendered_count = 0,
             .spacing        = 0 };
  ex     = {};
  REQUIRE( f() == ex );

  config = { .tile           = e_tile::commodity_tobacco_20,
             .rendered_count = 0,
             .spacing        = 4 };
  ex     = {};
  REQUIRE( f() == ex );

  config = { .tile           = e_tile::commodity_tobacco_20,
             .rendered_count = 1,
             .spacing        = 4 };
  ex     = {};
  ex.tiles.resize( 1 );
  ex.tiles[0].tile  = e_tile::commodity_tobacco_20;
  ex.tiles[0].where = { .x = -4, .y = 0 };
  ex.bounds.origin  = { .x = 0, .y = 0 };
  ex.bounds.size    = { .w = 13, .h = 20 };
  REQUIRE( f() == ex );

  config = { .tile           = e_tile::commodity_tobacco_20,
             .rendered_count = 4,
             .spacing        = 4 };
  ex     = {};
  ex.tiles.resize( 4 );
  ex.tiles[0].tile  = e_tile::commodity_tobacco_20;
  ex.tiles[0].where = { .x = -4, .y = 0 };
  ex.tiles[1].tile  = e_tile::commodity_tobacco_20;
  ex.tiles[1].where = { .x = 0, .y = 0 };
  ex.tiles[2].tile  = e_tile::commodity_tobacco_20;
  ex.tiles[2].where = { .x = 4, .y = 0 };
  ex.tiles[3].tile  = e_tile::commodity_tobacco_20;
  ex.tiles[3].where = { .x = 8, .y = 0 };
  ex.bounds.origin  = { .x = 0, .y = 0 };
  ex.bounds.size    = { .w = 4 * 3 + 13, .h = 20 };
  REQUIRE( f() == ex );
}

TEST_CASE( "[spread-builder] build_tile_spread_multi" ) {
  rr::MockTextometer textometer;
  TileSpreadConfigMulti in;
  TileSpreadRenderPlans ex;

  auto const f = [&] {
    return build_tile_spread_multi( textometer, in );
  };

  testing_set_trimmed_cache(
      e_tile::commodity_tobacco_20,
      rect{ .origin = { .x = 4, .y = 1 },
            .size   = { .w = 13, .h = 19 } } );
  testing_set_trimmed_cache(
      e_tile::red_x_12, rect{ .origin = { .x = 2, .y = 2 },
                              .size   = { .w = 14, .h = 14 } } );

  using enum e_tile;

  // Default.
  in = {};
  ex = {};
  REQUIRE( f() == ex );

  in = {};
  in.tiles.resize( 1 );
  in.tiles[0].tile           = commodity_tobacco_20;
  in.tiles[0].count          = 1;
  in.tiles[0].red_xs         = nothing;
  in.tiles[0].label_override = nothing;
  in.options.bounds          = 30;
  in.options.label_policy    = SpreadLabels::never{};
  in.options.label_opts      = {};
  in.group_spacing           = 5;

  ex = { .bounds = { .w = 13, .h = 20 },
         .plans  = { { .bounds = { .origin = { .x = 0, .y = 0 },
                                   .size = { .w = 13, .h = 20 } },
                       .tiles  = { { .tile  = commodity_tobacco_20,
                                     .where = { .x = -4, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false } },
                       .labels = {} } } };
  REQUIRE( f() == ex );

  in = {};
  in.tiles.resize( 1 );
  in.tiles[0].tile           = commodity_tobacco_20;
  in.tiles[0].count          = 4;
  in.tiles[0].red_xs         = nothing;
  in.tiles[0].label_override = nothing;
  in.options.bounds          = 30;
  in.options.label_policy    = SpreadLabels::never{};
  in.options.label_opts      = {};
  in.group_spacing           = 5;

  ex = { .bounds = { .w = 28, .h = 20 },
         .plans  = { { .bounds = { .origin = { .x = 0, .y = 0 },
                                   .size = { .w = 28, .h = 20 } },
                       .tiles  = { { .tile  = commodity_tobacco_20,
                                     .where = { .x = -4, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 1, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 6, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 11, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false } },
                       .labels = {} } } };
  REQUIRE( f() == ex );

  in = {};
  in.tiles.resize( 1 );
  in.tiles[0].tile           = commodity_tobacco_20;
  in.tiles[0].count          = 40;
  in.tiles[0].red_xs         = nothing;
  in.tiles[0].label_override = nothing;
  in.options.bounds          = 30;
  in.options.label_policy    = SpreadLabels::never{};
  in.options.label_opts      = {};
  in.group_spacing           = 5;

  ex = { .bounds = { .w = 30, .h = 20 },
         .plans  = { { .bounds = { .origin = { .x = 0, .y = 0 },
                                   .size = { .w = 30, .h = 20 } },
                       .tiles  = { { .tile  = commodity_tobacco_20,
                                     .where = { .x = -4, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = -3, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = -2, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = -1, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 0, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 1, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 2, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 3, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 4, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 5, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 6, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 7, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 8, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 9, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 10, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 11, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 12, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 13, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false } },
                       .labels = {} } } };
  REQUIRE( f() == ex );

  in = {};
  in.tiles.resize( 1 );
  in.tiles[0].tile   = commodity_tobacco_20;
  in.tiles[0].count  = 4;
  in.tiles[0].red_xs = SpreadXs{ .starting_position = 2 };
  in.tiles[0].label_override = nothing;
  in.options.bounds          = 30;
  in.options.label_policy    = SpreadLabels::never{};
  in.options.label_opts      = {};
  in.group_spacing           = 5;

  ex = { .bounds = { .w = 28, .h = 20 },
         .plans  = { { .bounds = { .origin = { .x = 0, .y = 0 },
                                   .size = { .w = 28, .h = 20 } },
                       .tiles  = { { .tile  = commodity_tobacco_20,
                                     .where = { .x = -4, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 1, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 6, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = red_x_12,
                                     .where = { .x = 8, .y = 2 },
                                     .is_overlay = true,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 11, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = red_x_12,
                                     .where = { .x = 13, .y = 2 },
                                     .is_overlay = true,
                                     .is_greyed  = false } },
                       .labels = {} } } };
  REQUIRE( f() == ex );

  in = {};
  in.tiles.resize( 1 );
  in.tiles[0].tile   = commodity_tobacco_20;
  in.tiles[0].count  = 3;
  in.tiles[0].red_xs = SpreadXs{ .starting_position = 0 };
  in.tiles[0].label_override = nothing;
  in.options.bounds          = 14;
  in.options.label_policy    = SpreadLabels::always{};
  in.options.label_opts      = {};
  in.group_spacing           = 5;

  ex = {
    .bounds = { .w = 14, .h = 20 },
    .plans  = {
      { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 14, .h = 20 } },
         .tiles  = { { .tile       = commodity_tobacco_20,
                       .where      = { .x = -4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = red_x_12,
                       .where      = { .x = -2, .y = 2 },
                       .is_overlay = true,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = -3, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = red_x_12,
                       .where      = { .x = -1, .y = 2 },
                       .is_overlay = true,
                       .is_greyed  = false } },
         .labels = {
          { .options = { .color_fg =
                              pixel::from_hex_rgba( 0xFF0000FF ),
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "3",
             .bounds  = { .origin = { .x = 0, .y = 0 },
                          .size = { .w = 10, .h = 12 } } } } } } };
  textometer
      .EXPECT__dimensions_for_line(
          rr::TextLayout{ .monospace    = false,
                          .spacing      = nothing,
                          .line_spacing = nothing },
          "3" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  in = {};
  in.tiles.resize( 1 );
  in.tiles[0].tile   = commodity_tobacco_20;
  in.tiles[0].count  = 40;
  in.tiles[0].red_xs = SpreadXs{ .starting_position = 20 };
  in.tiles[0].label_override = nothing;
  in.options.bounds          = 30;
  in.options.label_policy    = SpreadLabels::never{};
  in.options.label_opts      = {};
  in.group_spacing           = 5;

  ex = { .bounds = { .w = 30, .h = 20 },
         .plans  = { { .bounds = { .origin = { .x = 0, .y = 0 },
                                   .size = { .w = 30, .h = 20 } },
                       .tiles  = { { .tile  = commodity_tobacco_20,
                                     .where = { .x = -4, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = -3, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = -2, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = -1, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 0, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 1, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 2, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 3, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 4, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 5, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = red_x_12,
                                     .where = { .x = 7, .y = 2 },
                                     .is_overlay = true,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 6, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = red_x_12,
                                     .where = { .x = 8, .y = 2 },
                                     .is_overlay = true,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 7, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = red_x_12,
                                     .where = { .x = 9, .y = 2 },
                                     .is_overlay = true,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 8, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = red_x_12,
                                     .where = { .x = 10, .y = 2 },
                                     .is_overlay = true,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 9, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = red_x_12,
                                     .where = { .x = 11, .y = 2 },
                                     .is_overlay = true,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 10, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = red_x_12,
                                     .where = { .x = 12, .y = 2 },
                                     .is_overlay = true,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 11, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = red_x_12,
                                     .where = { .x = 13, .y = 2 },
                                     .is_overlay = true,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 12, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = red_x_12,
                                     .where = { .x = 14, .y = 2 },
                                     .is_overlay = true,
                                     .is_greyed  = false },
                                   { .tile  = commodity_tobacco_20,
                                     .where = { .x = 13, .y = 0 },
                                     .is_overlay = false,
                                     .is_greyed  = false },
                                   { .tile  = red_x_12,
                                     .where = { .x = 15, .y = 2 },
                                     .is_overlay = true,
                                     .is_greyed  = false } },
                       .labels = {} } } };
  REQUIRE( f() == ex );

  // Overlapping labels.
  in = {};
  in.tiles.resize( 2 );
  in.tiles[0].tile   = commodity_tobacco_20;
  in.tiles[1].tile   = commodity_tobacco_20;
  in.tiles[1].count  = 2;
  in.tiles[1].red_xs = SpreadXs{ .starting_position = 1 };
  in.tiles[1].label_override = nothing;
  in.options.bounds          = 30;
  in.options.label_policy    = SpreadLabels::always{};
  in.options.label_opts      = {};
  in.group_spacing           = 5;

  ex = {
    .bounds = { .w = 27, .h = 20 },
    .plans  = {
      { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 27, .h = 20 } },
         .tiles  = { { .tile       = commodity_tobacco_20,
                       .where      = { .x = -4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 10, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = red_x_12,
                       .where      = { .x = 12, .y = 2 },
                       .is_overlay = true,
                       .is_greyed  = false } },
         .labels = {
          { .options = { .color_fg     = nothing,
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "1",
             .bounds  = { .origin = { .x = 0, .y = 0 },
                          .size   = { .w = 10, .h = 12 } } },
          { .options = { .color_fg =
                              pixel::from_hex_rgba( 0xFF0000FF ),
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "1",
             .bounds  = { .origin = { .x = 14, .y = 0 },
                          .size = { .w = 10, .h = 12 } } } } } } };
  textometer
      .EXPECT__dimensions_for_line(
          rr::TextLayout{ .monospace    = false,
                          .spacing      = nothing,
                          .line_spacing = nothing },
          "1" )
      .times( 2 )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );
}

TEST_CASE( "[spread-builder] build_tile_spread" ) {
  rr::MockTextometer textometer;
  TileSpreadConfig in;
  TileSpreadRenderPlan ex;

  auto const f = [&] {
    return build_tile_spread( textometer, in );
  };

  testing_set_trimmed_cache(
      e_tile::commodity_tobacco_20,
      rect{ .origin = { .x = 4, .y = 1 },
            .size   = { .w = 13, .h = 19 } } );
  testing_set_trimmed_cache(
      e_tile::commodity_cigars_20,
      rect{ .origin = { .x = 7, .y = 2 },
            .size   = { .w = 5, .h = 16 } } );
  using enum e_tile;

  in                      = {};
  in.tile.tile            = commodity_tobacco_20;
  in.tile.count           = 4;
  in.tile.red_xs          = nothing;
  in.tile.label_override  = nothing;
  in.options.bounds       = 30;
  in.options.label_policy = SpreadLabels::never{};
  in.options.label_opts   = {};

  ex = { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 28, .h = 20 } },
         .tiles  = { { .tile       = commodity_tobacco_20,
                       .where      = { .x = -4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 1, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 6, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 11, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false } },
         .labels = {} };
  REQUIRE( f() == ex );

  in                      = {};
  in.tile.tile            = commodity_cigars_20;
  in.tile.count           = 4;
  in.tile.red_xs          = SpreadXs{ .starting_position = 0 };
  in.tile.label_override  = nothing;
  in.options.bounds       = 30;
  in.options.label_policy = SpreadLabels::always{};
  in.options.label_opts   = {};

  ex = { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 23, .h = 20 } },
         .tiles  = { { .tile       = commodity_cigars_20,
                       .where      = { .x = -7, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = red_x_6,
                       .where      = { .x = -6, .y = 2 },
                       .is_overlay = true,
                       .is_greyed  = false },
                     { .tile       = commodity_cigars_20,
                       .where      = { .x = -1, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = red_x_6,
                       .where      = { .x = 0, .y = 2 },
                       .is_overlay = true,
                       .is_greyed  = false },
                     { .tile       = commodity_cigars_20,
                       .where      = { .x = 5, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = red_x_6,
                       .where      = { .x = 6, .y = 2 },
                       .is_overlay = true,
                       .is_greyed  = false },
                     { .tile       = commodity_cigars_20,
                       .where      = { .x = 11, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = red_x_6,
                       .where      = { .x = 12, .y = 2 },
                       .is_overlay = true,
                       .is_greyed  = false } },
         .labels = {
           { .options = { .color_fg =
                              pixel::from_hex_rgba( 0xFF0000FF ),
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "4",
             .bounds  = { .origin = { .x = 0, .y = 0 },
                          .size   = { .w = 10, .h = 12 } } } } };
  textometer
      .EXPECT__dimensions_for_line(
          rr::TextLayout{ .monospace    = false,
                          .spacing      = nothing,
                          .line_spacing = nothing },
          "4" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // Overlapping labels.
  in                      = {};
  in.tile.tile            = commodity_cigars_20;
  in.tile.count           = 2;
  in.tile.red_xs          = SpreadXs{ .starting_position = 1 };
  in.tile.label_override  = nothing;
  in.options.bounds       = 10;
  in.options.label_policy = SpreadLabels::always{};
  in.options.label_opts   = {};

  ex = { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 10, .h = 20 } },
         .tiles  = { { .tile       = commodity_cigars_20,
                       .where      = { .x = -7, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_cigars_20,
                       .where      = { .x = -2, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = red_x_6,
                       .where      = { .x = -1, .y = 2 },
                       .is_overlay = true,
                       .is_greyed  = false } },
         .labels = {
           { .options = { .color_fg     = nothing,
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "1",
             .bounds  = { .origin = { .x = 0, .y = 0 },
                          .size   = { .w = 10, .h = 12 } } },
           { .options = { .color_fg =
                              pixel::from_hex_rgba( 0xFF0000FF ),
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "1",
             .bounds  = { .origin = { .x = 11, .y = 0 },
                          .size   = { .w = 10, .h = 12 } } } } };
  textometer
      .EXPECT__dimensions_for_line(
          rr::TextLayout{ .monospace    = false,
                          .spacing      = nothing,
                          .line_spacing = nothing },
          "1" )
      .times( 2 )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );
}

TEST_CASE( "[spread-builder] build_progress_tile_spread" ) {
  rr::MockTextometer textometer;
  ProgressTileSpreadConfig in;
  TileSpreadRenderPlan ex;

  auto const f = [&] {
    return build_progress_tile_spread( textometer, in );
  };

  testing_set_trimmed_cache(
      e_tile::commodity_tobacco_20,
      rect{ .origin = { .x = 4, .y = 1 },
            .size   = { .w = 13, .h = 19 } } );
  using enum e_tile;

  in                      = {};
  in.tile                 = commodity_tobacco_20;
  in.count                = 10;
  in.progress_count       = 6;
  in.label_override       = nothing;
  in.options.bounds       = 30;
  in.options.label_policy = SpreadLabels::always{};
  in.options.label_opts   = {};

  ex = { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 22, .h = 20 } },
         .tiles  = { { .tile       = commodity_tobacco_20,
                       .where      = { .x = -4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = -3, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 0, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 1, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 5, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false } },
         .labels = {
           { .options = { .color_fg     = nothing,
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "6",
             .bounds  = { .origin = { .x = 0, .y = 0 },
                          .size   = { .w = 10, .h = 12 } } } } };
  textometer
      .EXPECT__dimensions_for_line(
          rr::TextLayout{ .monospace    = false,
                          .spacing      = nothing,
                          .line_spacing = nothing },
          "6" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // label override.
  in                      = {};
  in.tile                 = commodity_tobacco_20;
  in.count                = 10;
  in.progress_count       = 6;
  in.label_override       = 5;
  in.options.bounds       = 30;
  in.options.label_policy = SpreadLabels::always{};
  in.options.label_opts   = {};

  ex = { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 22, .h = 20 } },
         .tiles  = { { .tile       = commodity_tobacco_20,
                       .where      = { .x = -4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = -3, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 0, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 1, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 5, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false } },
         .labels = {
           { .options = { .color_fg     = nothing,
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "5",
             .bounds  = { .origin = { .x = 0, .y = 0 },
                          .size   = { .w = 10, .h = 12 } } } } };
  textometer
      .EXPECT__dimensions_for_line(
          rr::TextLayout{ .monospace    = false,
                          .spacing      = nothing,
                          .line_spacing = nothing },
          "5" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  in                      = {};
  in.tile                 = commodity_tobacco_20;
  in.count                = 100;
  in.progress_count       = 60;
  in.label_override       = nothing;
  in.options.bounds       = 30;
  in.options.label_policy = SpreadLabels::always{};
  in.options.label_opts   = {};

  ex = { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 22, .h = 20 } },
         .tiles  = { { .tile       = commodity_tobacco_20,
                       .where      = { .x = -4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = -3, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = -2, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = -1, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 0, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 1, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 2, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 3, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 5, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false } },
         .labels = {
           { .options = { .color_fg     = nothing,
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "60",
             .bounds  = { .origin = { .x = 0, .y = 0 },
                          .size   = { .w = 16, .h = 12 } } } } };
  textometer
      .EXPECT__dimensions_for_line(
          rr::TextLayout{ .monospace    = false,
                          .spacing      = nothing,
                          .line_spacing = nothing },
          "60" )
      .returns( size{ .w = 6 * 2, .h = 8 } );
  REQUIRE( f() == ex );

  // Label override.
  in                      = {};
  in.tile                 = commodity_tobacco_20;
  in.count                = 100;
  in.progress_count       = 60;
  in.label_override       = 59;
  in.options.bounds       = 30;
  in.options.label_policy = SpreadLabels::always{};
  in.options.label_opts   = {};

  ex = { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 22, .h = 20 } },
         .tiles  = { { .tile       = commodity_tobacco_20,
                       .where      = { .x = -4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = -3, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = -2, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = -1, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 0, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 1, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 2, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 3, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 5, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false } },
         .labels = {
           { .options = { .color_fg     = nothing,
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "59",
             .bounds  = { .origin = { .x = 0, .y = 0 },
                          .size   = { .w = 16, .h = 12 } } } } };
  textometer
      .EXPECT__dimensions_for_line(
          rr::TextLayout{ .monospace    = false,
                          .spacing      = nothing,
                          .line_spacing = nothing },
          "59" )
      .returns( size{ .w = 6 * 2, .h = 8 } );
  REQUIRE( f() == ex );
}

TEST_CASE( "[spread-builder] choose_x_tile_for" ) {
  using enum e_tile;

  auto const f = []( e_tile const in ) {
    return detail::choose_x_tile_for( in );
  };

  auto const register_as_w = []( e_tile const tile,
                                 int const w ) {
    testing_set_trimmed_cache(
        tile, rect{ .origin = { .x = 0, .y = 0 },
                    .size   = { .w = w, .h = 1 } } );
  };

  register_as_w( commodity_food_16, 0 );
  register_as_w( commodity_sugar_16, 1 );
  register_as_w( commodity_tobacco_16, 2 );
  register_as_w( commodity_cotton_16, 3 );
  register_as_w( commodity_furs_16, 4 );
  register_as_w( commodity_lumber_16, 5 );
  register_as_w( commodity_ore_16, 6 );
  register_as_w( commodity_silver_16, 7 );
  register_as_w( commodity_horses_16, 8 );
  register_as_w( commodity_rum_16, 9 );
  register_as_w( commodity_cigars_16, 10 );
  register_as_w( commodity_cloth_16, 11 );
  register_as_w( commodity_coats_16, 12 );
  register_as_w( commodity_trade_goods_16, 13 );
  register_as_w( commodity_tools_16, 14 );
  register_as_w( commodity_muskets_16, 15 );

  REQUIRE( f( commodity_food_16 ) == e_tile::red_x_6 );
  REQUIRE( f( commodity_sugar_16 ) == e_tile::red_x_6 );
  REQUIRE( f( commodity_tobacco_16 ) == e_tile::red_x_6 );
  REQUIRE( f( commodity_cotton_16 ) == e_tile::red_x_6 );
  REQUIRE( f( commodity_furs_16 ) == e_tile::red_x_6 );
  REQUIRE( f( commodity_lumber_16 ) == e_tile::red_x_6 );
  REQUIRE( f( commodity_ore_16 ) == e_tile::red_x_6 );
  REQUIRE( f( commodity_silver_16 ) == e_tile::red_x_6 );
  REQUIRE( f( commodity_horses_16 ) == e_tile::red_x_8 );
  REQUIRE( f( commodity_rum_16 ) == e_tile::red_x_8 );
  REQUIRE( f( commodity_cigars_16 ) == e_tile::red_x_8 );
  REQUIRE( f( commodity_cloth_16 ) == e_tile::red_x_8 );
  REQUIRE( f( commodity_coats_16 ) == e_tile::red_x_12 );
  REQUIRE( f( commodity_trade_goods_16 ) == e_tile::red_x_12 );
  REQUIRE( f( commodity_tools_16 ) == e_tile::red_x_12 );
  REQUIRE( f( commodity_muskets_16 ) == e_tile::red_x_12 );
}

TEST_CASE( "[spread-builder] build_inhomogeneous_tile_spread" ) {
  rr::MockTextometer textometer;
  InhomogeneousTileSpreadConfig in;
  TileSpreadRenderPlan ex;

  auto const f = [&] {
    return build_inhomogeneous_tile_spread( textometer, in );
  };

  testing_set_trimmed_cache(
      e_tile::commodity_tobacco_20,
      rect{ .origin = { .x = 4, .y = 1 },
            .size   = { .w = 13, .h = 19 } } );
  testing_set_trimmed_cache(
      e_tile::commodity_cigars_20,
      rect{ .origin = { .x = 7, .y = 2 },
            .size   = { .w = 5, .h = 16 } } );
  testing_set_trimmed_cache(
      e_tile::commodity_horses_20,
      rect{ .origin = { .x = 1, .y = 3 },
            .size   = { .w = 18, .h = 16 } } );
  testing_set_trimmed_cache(
      e_tile::commodity_muskets_20,
      rect{ .origin = { .x = 8, .y = 1 },
            .size   = { .w = 6, .h = 18 } } );
  // NOTE: Empty width.
  testing_set_trimmed_cache(
      e_tile::commodity_food_20,
      rect{ .origin = { .x = 8, .y = 1 },
            .size   = { .w = 0, .h = 18 } } );
  using enum e_tile;

  // Default.
  in = {};
  ex = {};
  REQUIRE( f() == ex );

  in = {};
  in.tiles.resize( 4 );
  in.tiles[0].tile        = commodity_tobacco_20;
  in.tiles[1].tile        = commodity_cigars_20;
  in.tiles[1].greyed      = true;
  in.tiles[2].tile        = commodity_horses_20;
  in.tiles[3].tile        = commodity_muskets_20;
  in.max_spacing          = nothing;
  in.sort_tiles           = false;
  in.options.bounds       = 30;
  in.options.label_policy = SpreadLabels::always{};
  in.options.label_opts   = {};

  ex = { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 28, .h = 20 } },
         .tiles  = { { .tile       = commodity_tobacco_20,
                       .where      = { .x = -4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_cigars_20,
                       .where      = { .x = 2, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = true },
                     { .tile       = commodity_horses_20,
                       .where      = { .x = 8, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_muskets_20,
                       .where      = { .x = 14, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false } },
         .labels = {
           { .options = { .color_fg     = nothing,
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "4",
             .bounds  = { .origin = { .x = 0, .y = 0 },
                          .size   = { .w = 10, .h = 12 } } } } };
  textometer
      .EXPECT__dimensions_for_line(
          rr::TextLayout{ .monospace    = false,
                          .spacing      = nothing,
                          .line_spacing = nothing },
          "4" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // Sorted.
  in = {};
  in.tiles.resize( 4 );
  in.tiles[0].tile        = commodity_tobacco_20;
  in.tiles[1].tile        = commodity_cigars_20;
  in.tiles[1].greyed      = true;
  in.tiles[2].tile        = commodity_horses_20;
  in.tiles[3].tile        = commodity_muskets_20;
  in.max_spacing          = nothing;
  in.sort_tiles           = true;
  in.options.bounds       = 30;
  in.options.label_policy = SpreadLabels::always{};
  in.options.label_opts   = {};

  ex = { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 29, .h = 20 } },
         .tiles  = { { .tile       = commodity_horses_20,
                       .where      = { .x = -1, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_tobacco_20,
                       .where      = { .x = 5, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_muskets_20,
                       .where      = { .x = 11, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_cigars_20,
                       .where      = { .x = 17, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = true } },
         .labels = {
           { .options = { .color_fg     = nothing,
                          .color_bg     = nothing,
                          .placement    = nothing,
                          .text_padding = nothing },
             .text    = "4",
             .bounds  = { .origin = { .x = 0, .y = 0 },
                          .size   = { .w = 10, .h = 12 } } } } };
  textometer
      .EXPECT__dimensions_for_line(
          rr::TextLayout{ .monospace    = false,
                          .spacing      = nothing,
                          .line_spacing = nothing },
          "4" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // Empty widths.
  in = {};
  in.tiles.resize( 4 );
  in.tiles[0].tile        = commodity_food_20;
  in.tiles[1].tile        = commodity_food_20;
  in.tiles[1].greyed      = true;
  in.tiles[2].tile        = commodity_food_20;
  in.tiles[3].tile        = commodity_food_20;
  in.max_spacing          = nothing;
  in.sort_tiles           = false;
  in.options.bounds       = 100;
  in.options.label_policy = SpreadLabels::never{};
  in.options.label_opts   = {};

  ex = { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 3, .h = 19 } },
         .tiles  = { { .tile       = commodity_food_20,
                       .where      = { .x = -8, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_food_20,
                       .where      = { .x = -7, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = true },
                     { .tile       = commodity_food_20,
                       .where      = { .x = -6, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false },
                     { .tile       = commodity_food_20,
                       .where      = { .x = -5, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false } },
         .labels = {} };
  REQUIRE( f() == ex );

  // Single.
  in = {};
  in.tiles.resize( 1 );
  in.tiles[0].tile        = commodity_tobacco_20;
  in.max_spacing          = nothing;
  in.sort_tiles           = false;
  in.options.bounds       = 100;
  in.options.label_policy = SpreadLabels::never{};
  in.options.label_opts   = {};

  ex = { .bounds = { .origin = { .x = 0, .y = 0 },
                     .size   = { .w = 13, .h = 20 } },
         .tiles  = { { .tile       = commodity_tobacco_20,
                       .where      = { .x = -4, .y = 0 },
                       .is_overlay = false,
                       .is_greyed  = false } },
         .labels = {} };
  REQUIRE( f() == ex );
}

} // namespace
} // namespace rn
