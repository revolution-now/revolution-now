/****************************************************************
**spread-render-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-20.
*
* Description: Unit tests for the spread-render module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/spread-render.hpp"

// Testing
#include "test/mocking.hpp"
#include "test/mocks/render/itextometer.hpp"

// Revolution Now
#include "src/tiles.hpp"

// config
#include "src/config/tile-enum.rds.hpp"

// refl
#include "src/refl/query-enum.hpp"
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::base::nothing;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;
using ::refl::enum_count;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[spread] render_plan_for_tile_spread" ) {
  TileSpreadSpecs in;
  TileSpreadRenderPlans ex;
  rr::MockTextometer textometer;

  auto f = [&] {
    return render_plan_for_tile_spread( textometer, in );
  };

  // Default.
  in = {};
  ex = {};
  REQUIRE( f() == ex );

  in               = {};
  in.group_spacing = 1;
  in.spreads.resize( 1 );
  in.spreads[0].algo_spec.count                      = 0;
  in.spreads[0].algo_spec.trimmed.start              = 3;
  in.spreads[0].algo_spec.trimmed.len                = 10;
  in.spreads[0].tile_spec.icon_spread.rendered_count = 0;
  in.spreads[0].tile_spec.icon_spread.spacing        = 1;
  in.spreads[0].tile_spec.tile = e_tile::dragoon;

  ex = {};
  REQUIRE( f() == ex );

  in               = {};
  in.group_spacing = 1;
  in.spreads.resize( 1 );
  in.spreads[0].algo_spec.count                      = 1;
  in.spreads[0].algo_spec.trimmed.start              = 3;
  in.spreads[0].algo_spec.trimmed.len                = 10;
  in.spreads[0].tile_spec.icon_spread.rendered_count = 1;
  in.spreads[0].tile_spec.icon_spread.spacing        = 1;
  in.spreads[0].tile_spec.tile = e_tile::dragoon;

  ex        = {};
  ex.bounds = { .w = 10, .h = 32 };
  ex.plans.resize( 1 );
  ex.plans[0].bounds.origin = { .x = 0, .y = 0 };
  ex.plans[0].bounds.size   = { .w = 10, .h = 32 };
  ex.plans[0].tiles.resize( 1 );
  ex.plans[0].tiles[0].tile  = e_tile::dragoon;
  ex.plans[0].tiles[0].where = point{ .x = -3, .y = 0 };
  REQUIRE( f() == ex );

  in               = {};
  in.group_spacing = 1;
  in.spreads.resize( 1 );
  in.spreads[0].algo_spec.count                      = 3;
  in.spreads[0].algo_spec.trimmed.start              = 3;
  in.spreads[0].algo_spec.trimmed.len                = 10;
  in.spreads[0].tile_spec.icon_spread.rendered_count = 3;
  in.spreads[0].tile_spec.icon_spread.spacing        = 2;
  in.spreads[0].tile_spec.tile = e_tile::dragoon;

  ex        = {};
  ex.bounds = { .w = 2 + 2 + 10, .h = 32 };
  ex.plans.resize( 1 );
  ex.plans[0].bounds.origin = { .x = 0, .y = 0 };
  ex.plans[0].bounds.size   = { .w = 2 + 2 + 10, .h = 32 };
  ex.plans[0].tiles.resize( 3 );
  ex.plans[0].tiles[0].tile  = e_tile::dragoon;
  ex.plans[0].tiles[0].where = point{ .x = -3, .y = 0 };
  ex.plans[0].tiles[1].tile  = e_tile::dragoon;
  ex.plans[0].tiles[1].where = point{ .x = -1, .y = 0 };
  ex.plans[0].tiles[2].tile  = e_tile::dragoon;
  ex.plans[0].tiles[2].where = point{ .x = 1, .y = 0 };
  REQUIRE( f() == ex );

  in               = {};
  in.group_spacing = 3;
  in.spreads.resize( 3 );
  in.spreads[0].algo_spec.count                      = 3;
  in.spreads[0].algo_spec.trimmed.start              = 3;
  in.spreads[0].algo_spec.trimmed.len                = 10;
  in.spreads[0].tile_spec.icon_spread.rendered_count = 3;
  in.spreads[0].tile_spec.icon_spread.spacing        = 2;
  in.spreads[0].tile_spec.tile          = e_tile::dragoon;
  in.spreads[1].algo_spec.count         = 2;
  in.spreads[1].algo_spec.trimmed.start = 2;
  in.spreads[1].algo_spec.trimmed.len   = 5;
  in.spreads[1].tile_spec.icon_spread.rendered_count = 2;
  in.spreads[1].tile_spec.icon_spread.spacing        = 3;
  in.spreads[1].tile_spec.tile          = e_tile::soldier;
  in.spreads[2].algo_spec.count         = 3;
  in.spreads[2].algo_spec.trimmed.start = 0;
  in.spreads[2].algo_spec.trimmed.len   = 5;
  in.spreads[2].tile_spec.icon_spread.rendered_count = 2;
  in.spreads[2].tile_spec.icon_spread.spacing        = 1;
  in.spreads[2].tile_spec.tile = e_tile::free_colonist;

  ex        = {};
  ex.bounds = { .w = 2 + 2 + 10 + 3 + 3 + 5 + 3 + 1 + 5,
                .h = 32 };
  ex.plans.resize( 3 );
  ex.plans[0].bounds.origin = { .x = 0, .y = 0 };
  ex.plans[0].bounds.size   = { .w = 2 + 2 + 10, .h = 32 };
  ex.plans[0].tiles.resize( 3 );
  ex.plans[0].tiles[0].tile  = e_tile::dragoon;
  ex.plans[0].tiles[0].where = point{ .x = -3, .y = 0 };
  ex.plans[0].tiles[1].tile  = e_tile::dragoon;
  ex.plans[0].tiles[1].where = point{ .x = -1, .y = 0 };
  ex.plans[0].tiles[2].tile  = e_tile::dragoon;
  ex.plans[0].tiles[2].where = point{ .x = 1, .y = 0 };
  ex.plans[1].bounds.origin  = { .x = 14 + 3, .y = 0 };
  ex.plans[1].bounds.size    = { .w = 3 + 5, .h = 32 };
  ex.plans[1].tiles.resize( 2 );
  ex.plans[1].tiles[0].tile  = e_tile::soldier;
  ex.plans[1].tiles[0].where = point{ .x = 17 - 2, .y = 0 };
  ex.plans[1].tiles[1].tile  = e_tile::soldier;
  ex.plans[1].tiles[1].where = point{ .x = 17 + 1, .y = 0 };
  ex.plans[2].bounds.origin  = { .x = 25 + 3, .y = 0 };
  ex.plans[2].bounds.size    = { .w = 1 + 5, .h = 32 };
  ex.plans[2].tiles.resize( 2 );
  ex.plans[2].tiles[0].tile  = e_tile::free_colonist;
  ex.plans[2].tiles[0].where = point{ .x = 28 - 0, .y = 0 };
  ex.plans[2].tiles[1].tile  = e_tile::free_colonist;
  ex.plans[2].tiles[1].where = point{ .x = 28 + 1, .y = 0 };
  REQUIRE( f() == ex );

  // One spread, one tile, with label.
  in               = {};
  in.group_spacing = 1;
  in.label_policy  = SpreadLabels::always{};
  in.spreads.resize( 1 );
  in.spreads[0].algo_spec.count                      = 1;
  in.spreads[0].algo_spec.trimmed.start              = 2;
  in.spreads[0].algo_spec.trimmed.len                = 28;
  in.spreads[0].tile_spec.icon_spread.rendered_count = 1;
  in.spreads[0].tile_spec.icon_spread.spacing        = 1;
  in.spreads[0].tile_spec.tile = e_tile::dragoon;

  ex        = {};
  ex.bounds = { .w = 28, .h = 32 };
  ex.plans.resize( 1 );
  ex.plans[0].bounds.origin = { .x = 0, .y = 0 };
  ex.plans[0].bounds.size   = { .w = 28, .h = 32 };
  ex.plans[0].tiles.resize( 1 );
  ex.plans[0].tiles[0].tile  = e_tile::dragoon;
  ex.plans[0].tiles[0].where = point{ .x = -2, .y = 0 };
  ex.plans[0].labels.resize( 1 );
  ex.plans[0].labels[0].text          = "1";
  ex.plans[0].labels[0].bounds.origin = { .x = 0, .y = 0 };
  ex.plans[0].labels[0].bounds.size   = { .w = 10, .h = 12 };
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "1" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // One spread, one tile, with label (auto_decide).
  in               = {};
  in.group_spacing = 1;
  in.label_policy  = SpreadLabels::auto_decide{};
  in.spreads.resize( 1 );
  in.spreads[0].algo_spec.count                      = 1;
  in.spreads[0].algo_spec.trimmed.start              = 2;
  in.spreads[0].algo_spec.trimmed.len                = 28;
  in.spreads[0].tile_spec.icon_spread.rendered_count = 1;
  in.spreads[0].tile_spec.icon_spread.spacing        = 1;
  in.spreads[0].tile_spec.tile = e_tile::dragoon;

  ex        = {};
  ex.bounds = { .w = 28, .h = 32 };
  ex.plans.resize( 1 );
  ex.plans[0].bounds.origin = { .x = 0, .y = 0 };
  ex.plans[0].bounds.size   = { .w = 28, .h = 32 };
  ex.plans[0].tiles.resize( 1 );
  ex.plans[0].tiles[0].tile  = e_tile::dragoon;
  ex.plans[0].tiles[0].where = point{ .x = -2, .y = 0 };
  REQUIRE( f() == ex );

  // One spread, two tiles, with label (auto_decide).
  in               = {};
  in.group_spacing = 1;
  in.label_policy  = SpreadLabels::auto_decide{};
  in.spreads.resize( 1 );
  in.spreads[0].algo_spec.count                      = 2;
  in.spreads[0].algo_spec.trimmed.start              = 2;
  in.spreads[0].algo_spec.trimmed.len                = 28;
  in.spreads[0].tile_spec.icon_spread.rendered_count = 2;
  in.spreads[0].tile_spec.icon_spread.spacing        = 1;
  in.spreads[0].tile_spec.tile = e_tile::dragoon;

  ex        = {};
  ex.bounds = { .w = 1 + 28, .h = 32 };
  ex.plans.resize( 1 );
  ex.plans[0].bounds.origin = { .x = 0, .y = 0 };
  ex.plans[0].bounds.size   = { .w = 1 + 28, .h = 32 };
  ex.plans[0].tiles.resize( 2 );
  ex.plans[0].tiles[0].tile  = e_tile::dragoon;
  ex.plans[0].tiles[0].where = point{ .x = -2, .y = 0 };
  ex.plans[0].tiles[1].tile  = e_tile::dragoon;
  ex.plans[0].tiles[1].where = point{ .x = -1, .y = 0 };
  ex.plans[0].labels.resize( 1 );
  ex.plans[0].labels[0].text          = "2";
  ex.plans[0].labels[0].bounds.origin = { .x = 0, .y = 0 };
  ex.plans[0].labels[0].bounds.size   = { .w = 10, .h = 12 };
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "2" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // One spread, one tile, with label (left_middle_adjusted).
  in               = {};
  in.group_spacing = 1;
  in.label_policy  = SpreadLabels::always{};
  in.spreads.resize( 1 );
  in.spreads[0].algo_spec.count                      = 1;
  in.spreads[0].algo_spec.trimmed.start              = 2;
  in.spreads[0].algo_spec.trimmed.len                = 28;
  in.spreads[0].tile_spec.icon_spread.rendered_count = 1;
  in.spreads[0].tile_spec.icon_spread.spacing        = 1;
  in.spreads[0].tile_spec.tile = e_tile::dragoon;
  in.spreads[0].tile_spec.label_opts.placement =
      SpreadLabelPlacement::left_middle_adjusted{};

  ex        = {};
  ex.bounds = { .w = 28, .h = 32 };
  ex.plans.resize( 1 );
  ex.plans[0].bounds.origin = { .x = 0, .y = 0 };
  ex.plans[0].bounds.size   = { .w = 28, .h = 32 };
  ex.plans[0].tiles.resize( 1 );
  ex.plans[0].tiles[0].tile  = e_tile::dragoon;
  ex.plans[0].tiles[0].where = point{ .x = -2, .y = 0 };
  ex.plans[0].labels.resize( 1 );
  ex.plans[0].labels[0].text          = "1";
  ex.plans[0].labels[0].bounds.origin = { .x = 9, .y = 10 };
  ex.plans[0].labels[0].bounds.size   = { .w = 10, .h = 12 };
  ex.plans[0].labels[0].options.placement =
      SpreadLabelPlacement::left_middle_adjusted{};
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "1" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // One spread, two tiles, with label (left_middle_adjusted).
  in               = {};
  in.group_spacing = 1;
  in.label_policy  = SpreadLabels::always{};
  in.spreads.resize( 1 );
  in.spreads[0].algo_spec.count                      = 2;
  in.spreads[0].algo_spec.trimmed.start              = 2;
  in.spreads[0].algo_spec.trimmed.len                = 28;
  in.spreads[0].tile_spec.icon_spread.rendered_count = 2;
  in.spreads[0].tile_spec.icon_spread.spacing        = 1;
  in.spreads[0].tile_spec.tile = e_tile::dragoon;
  in.spreads[0].tile_spec.label_opts.placement =
      SpreadLabelPlacement::left_middle_adjusted{};

  ex        = {};
  ex.bounds = { .w = 1 + 28, .h = 32 };
  ex.plans.resize( 1 );
  ex.plans[0].bounds.origin = { .x = 0, .y = 0 };
  ex.plans[0].bounds.size   = { .w = 1 + 28, .h = 32 };
  ex.plans[0].tiles.resize( 2 );
  ex.plans[0].tiles[0].tile  = e_tile::dragoon;
  ex.plans[0].tiles[0].where = point{ .x = -2, .y = 0 };
  ex.plans[0].tiles[1].tile  = e_tile::dragoon;
  ex.plans[0].tiles[1].where = point{ .x = -1, .y = 0 };
  ex.plans[0].labels.resize( 1 );
  ex.plans[0].labels[0].text          = "2";
  ex.plans[0].labels[0].bounds.origin = { .x = 0, .y = 10 };
  ex.plans[0].labels[0].bounds.size   = { .w = 10, .h = 12 };
  ex.plans[0].labels[0].options.placement =
      SpreadLabelPlacement::left_middle_adjusted{};
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "2" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // One spread, two tiles, with label (left_middle_adjusted,
  // large spacing).
  in               = {};
  in.group_spacing = 1;
  in.label_policy  = SpreadLabels::always{};
  in.spreads.resize( 1 );
  in.spreads[0].algo_spec.count                      = 2;
  in.spreads[0].algo_spec.trimmed.start              = 2;
  in.spreads[0].algo_spec.trimmed.len                = 28;
  in.spreads[0].tile_spec.icon_spread.rendered_count = 2;
  in.spreads[0].tile_spec.icon_spread.spacing        = 28;
  in.spreads[0].tile_spec.tile = e_tile::dragoon;
  in.spreads[0].tile_spec.label_opts.placement =
      SpreadLabelPlacement::left_middle_adjusted{};

  ex        = {};
  ex.bounds = { .w = 28 + 28, .h = 32 };
  ex.plans.resize( 1 );
  ex.plans[0].bounds.origin = { .x = 0, .y = 0 };
  ex.plans[0].bounds.size   = { .w = 28 + 28, .h = 32 };
  ex.plans[0].tiles.resize( 2 );
  ex.plans[0].tiles[0].tile  = e_tile::dragoon;
  ex.plans[0].tiles[0].where = point{ .x = -2, .y = 0 };
  ex.plans[0].tiles[1].tile  = e_tile::dragoon;
  ex.plans[0].tiles[1].where = point{ .x = 26, .y = 0 };
  ex.plans[0].labels.resize( 1 );
  ex.plans[0].labels[0].text          = "2";
  ex.plans[0].labels[0].bounds.origin = { .x = 9, .y = 10 };
  ex.plans[0].labels[0].bounds.size   = { .w = 10, .h = 12 };
  ex.plans[0].labels[0].options.placement =
      SpreadLabelPlacement::left_middle_adjusted{};
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "2" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // One spread, one tile, with label, non-default label posi-
  // tion.
  in = {
    .spreads =
        { { .algo_spec = SpreadSpec{ .count   = 1,
                                     .trimmed = { .start = 2,
                                                  .len = 28 } },
            .tile_spec =
                { .icon_spread = { .rendered_count = 1,
                                   .spacing        = 1 },
                  .tile        = e_tile::dragoon,
                  .label_opts =
                      SpreadLabelOptions{
                        .placement =
                            SpreadLabelPlacement::in_first_tile{
                              .placement =
                                  e_cdirection::sw } } } } },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{} };
  ex = {
    .bounds = { .w = 28, .h = 32 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 28, .h = 32 } },
         .tiles  = { { .tile  = e_tile::dragoon,
                       .where = { -2, 0 } } },
         .labels = { SpreadLabelRenderPlan{
           .options =
              SpreadLabelOptions{
                 .placement =
                    SpreadLabelPlacement::in_first_tile{
                       .placement = e_cdirection::sw } },
           .text   = "1",
           .bounds = { .origin = { .x = 0, .y = 32 - 8 - 4 },
                       .size   = { .w = 10, .h = 12 } },
        } } },
    } };
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "1" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // Label uses real count when different from rendered_count.
  in = {
    .spreads =
        { { .algo_spec = SpreadSpec{ .count   = 2,
                                     .trimmed = { .start = 2,
                                                  .len = 28 } },
            .tile_spec = { .icon_spread = { .rendered_count = 1,
                                            .spacing = 1 },
                           .tile        = e_tile::dragoon } } },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{} };
  ex = {
    .bounds = { .w = 28, .h = 32 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 28, .h = 32 } },
         .tiles  = { { .tile  = e_tile::dragoon,
                       .where = { -2, 0 } } },
         .labels = { SpreadLabelRenderPlan{
           .text   = "2",
           .bounds = { .origin = { .x = 0, .y = 0 },
                       .size   = { .w = 10, .h = 12 } },
        } } },
    } };
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "2" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // Three spreads, middle empty does not emit group spacing.
  in = {
    .spreads =
        {
          { .algo_spec = SpreadSpec{ .count   = 1,
                                     .trimmed = { .start = 2,
                                                  .len = 28 } },
            .tile_spec = { .icon_spread =
                               {

                                 .rendered_count = 1,
                                 .spacing        = 1 },
                           .tile = e_tile::dragoon } },
          { .algo_spec = SpreadSpec{ .count   = 0,
                                     .trimmed = { .start = 2,
                                                  .len = 28 } },
            .tile_spec = { .icon_spread =
                               {

                                 .rendered_count = 0,
                                 .spacing        = 1 },
                           .tile = e_tile::dragoon } },
          { .algo_spec = SpreadSpec{ .count   = 2,
                                     .trimmed = { .start = 11,
                                                  .len = 13 } },
            .tile_spec = { .icon_spread =
                               {

                                 .rendered_count = 2,
                                 .spacing        = 3 },
                           .tile = e_tile::soldier } },
        },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{},
  };
  ex = {
    .bounds = { .w = 28 + 1 + 16, .h = 32 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 28, .h = 32 } },
         .tiles =
             {
              { .tile = e_tile::dragoon, .where = { -2, 0 } },
            },
         .labels = { SpreadLabelRenderPlan{
           .text   = "1",
           .bounds = { .origin = { .x = 0, .y = 0 },
                       .size   = { .w = 10, .h = 12 } },
        } } },
      TileSpreadRenderPlan{
         .bounds = { .origin = { .x = 28 + 1 },
                     .size   = { .w = 13 + 3, .h = 32 } },
         .tiles =
             {
              { .tile = e_tile::soldier, .where = { 18, 0 } },
              { .tile = e_tile::soldier, .where = { 21, 0 } },
            },
         .labels = { SpreadLabelRenderPlan{
           .text   = "2",
           .bounds = { .origin = { .x = 29, .y = 0 },
                       .size   = { .w = 10, .h = 12 } },
        } },
      } } };
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "1" )
      .returns( size{ .w = 6, .h = 8 } );
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "2" )
      .returns( size{ .w = 6, .h = 8 } );
  REQUIRE( f() == ex );

  // One spread, one tile, with overlay, no label.
  testing_set_trimmed_cache(
      e_tile::red_x_12, rect{ .origin = { .x = 2, .y = 2 },
                              .size   = { .w = 14, .h = 14 } } );
  in = {
    .spreads       = { { .algo_spec =
                             SpreadSpec{
                               .count   = 1,
                               .trimmed = { .start = 2, .len = 28 } },
                         .tile_spec =
                             {
                               .icon_spread = { .rendered_count = 1,
                                                .spacing        = 1 },
                               .tile        = e_tile::dragoon,
                               .overlay_tile =
                             TileOverlay{
                                     .tile = e_tile::red_x_12,
                                     .starting_position = 0 },
                       } } },
    .group_spacing = 1,
    .label_policy  = {} };
  ex = {
    .bounds = { .w = 28, .h = 32 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 28, .h = 32 } },
         .tiles =
             {
              { .tile       = e_tile::dragoon,
                 .where      = { -2, 0 },
                 .is_overlay = false },
              { .tile       = e_tile::red_x_12,
                 .where      = { 6, 8 },
                 .is_overlay = true },
            },
         .labels = {} },
    } };
  REQUIRE( f() == ex );

  // One spread, two tiles, with overlay, no label, no overlap.
  in = {
    .spreads       = { { .algo_spec =
                             SpreadSpec{
                               .count   = 2,
                               .trimmed = { .start = 2, .len = 28 } },
                         .tile_spec =
                             {
                               .icon_spread = { .rendered_count = 2,
                                                .spacing        = 28 },
                               .tile        = e_tile::dragoon,
                               .overlay_tile =
                             TileOverlay{
                                     .tile = e_tile::red_x_12,
                                     .starting_position = 1 },
                       } } },
    .group_spacing = 1,
    .label_policy  = {} };
  ex = {
    .bounds = { .w = 56, .h = 32 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 56, .h = 32 } },
         .tiles =
             {
              { .tile       = e_tile::dragoon,
                 .where      = { -2, 0 },
                 .is_overlay = false },
              { .tile       = e_tile::dragoon,
                 .where      = { 26, 0 },
                 .is_overlay = false },
              { .tile       = e_tile::red_x_12,
                 .where      = { 28 + 6, 8 },
                 .is_overlay = true },
            },
         .labels = {} },
    } };
  REQUIRE( f() == ex );

  // One spread, two tiles, with overlay, no label, with overlap.
  in = {
    .spreads       = { { .algo_spec =
                             SpreadSpec{
                               .count   = 2,
                               .trimmed = { .start = 2, .len = 28 } },
                         .tile_spec =
                             {
                               .icon_spread = { .rendered_count = 2,
                                                .spacing        = 27 },
                               .tile        = e_tile::dragoon,
                               .overlay_tile =
                             TileOverlay{
                                     .tile = e_tile::red_x_12,
                                     .starting_position = 1 },
                       } } },
    .group_spacing = 1,
    .label_policy  = {} };
  ex = {
    .bounds = { .w = 55, .h = 32 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 55, .h = 32 } },
         .tiles =
             {
              { .tile       = e_tile::dragoon,
                 .where      = { -2, 0 },
                 .is_overlay = false },
              { .tile       = e_tile::dragoon,
                 .where      = { 25, 0 },
                 .is_overlay = false },
              { .tile       = e_tile::red_x_12,
                 .where      = { 25 + 2 - 2, 8 },
                 .is_overlay = true },
            },
         .labels = {} },
    } };
  REQUIRE( f() == ex );

  // One spread, one tile, with overlay, with label.
  testing_set_trimmed_cache(
      e_tile::red_x_12, rect{ .origin = { .x = 2, .y = 2 },
                              .size   = { .w = 12, .h = 12 } } );
  in = {
    .spreads =
        { { .algo_spec = SpreadSpec{ .count   = 1,
                                     .trimmed = { .start = 2,
                                                  .len = 28 } },
            .tile_spec =
                {
                  .icon_spread =
                      {

                        .rendered_count = 1, .spacing = 1 },
                  .tile = e_tile::dragoon,
                  .overlay_tile =
                      TileOverlay{ .tile = e_tile::red_x_12,
                                   .starting_position = 0 },
                } } },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{} };
  ex = {
    .bounds = { .w = 28, .h = 32 },
    .plans  = { TileSpreadRenderPlan{
       .bounds = { .origin = {}, .size = { .w = 28, .h = 32 } },
       .tiles =
           {
            { .tile       = e_tile::dragoon,
               .where      = { -2, 0 },
               .is_overlay = false },
            { .tile       = e_tile::red_x_12,
               .where      = { 6, 8 },
               .is_overlay = true },
          },
       .labels = { SpreadLabelRenderPlan{
         .options = SpreadLabelOptions{ .placement = nothing },
         .text    = "1",
         .bounds  = { .origin = { .x = 0, .y = 0 },
                      .size   = { .w = 16, .h = 12 } } } },
    } } };
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "1" )
      .returns( size{ .w = 6 * 2, .h = 8 } );
  REQUIRE( f() == ex );

  // One spread, one tile, with label, with line breaks.
  in = {
    .spreads =
        { { .algo_spec = SpreadSpec{ .count   = 10,
                                     .trimmed = { .start = 2,
                                                  .len = 10 } },
            .tile_spec =
                {
                  .icon_spread =
                      {

                        .rendered_count = 10, .spacing = 5 },
                  .tile = e_tile::commodity_food_20,
                  .label_opts =
                      SpreadLabelOptions{
                        .placement =
                            SpreadLabelPlacement::in_first_tile{
                              .placement = e_cdirection::sw } },
                } } },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{} };
  ex = {
    .bounds = { .w = 55, .h = 20 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 55, .h = 20 } },
         .tiles =
             {
              { .tile  = e_tile::commodity_food_20,
                 .where = { -2, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 3, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 8, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 13, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 18, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 23, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 28, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 33, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 38, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 43, 0 } },
            },
         .labels = { SpreadLabelRenderPlan{
           .options =
              SpreadLabelOptions{
                 .placement =
                    SpreadLabelPlacement::in_first_tile{
                       .placement = e_cdirection::sw } },
           .text   = "10",
           .bounds = { .origin = { .x = 0, .y = 20 - 12 },
                       .size   = { .w = 16, .h = 12 } },
        } } },
    } };
  textometer
      .EXPECT__dimensions_for_line( rr::TextLayout{}, "10" )
      .returns( size{ .w = 6 * 2, .h = 8 } );
  REQUIRE( f() == ex );

  // Test that a zero-count spread with a label requested does
  // not render a label.
  in = {
    .spreads =
        { { .algo_spec = SpreadSpec{ .count   = 0,
                                     .trimmed = { .start = 2,
                                                  .len = 28 } },

            .tile_spec = { .icon_spread = { .rendered_count = 0,
                                            .spacing = 1 },
                           .tile        = e_tile::dragoon } } },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{} };
  ex = {};
  REQUIRE( f() == ex );
}

TEST_CASE( "[spread] render_plan_for_tile_progress_spread" ) {
  ProgressTileSpreadSpec in;
  TileSpreadRenderPlan ex;
  rr::MockTextometer textometer;

  auto f = [&] {
    return render_plan_for_tile_progress_spread( textometer,
                                                 in );
  };

  // Default.
  in = {};
  ex = {};
  REQUIRE( f() == ex );

  // Two icons.
  in                                       = {};
  in.source_spec.bounds                    = 20;
  in.source_spec.spread_spec.count         = 2;
  in.source_spec.spread_spec.trimmed.start = 2;
  in.source_spec.spread_spec.trimmed.len   = 10;
  in.progress_spread.spacings.resize( 1 );
  in.progress_spread.spacings[0].mod     = 1;
  in.progress_spread.spacings[0].spacing = 1;
  in.rendered_count                      = 2;
  in.tile = e_tile::indentured_servant;

  ex               = {};
  ex.bounds.origin = { .x = 0, .y = 0 };
  ex.bounds.size   = { .w = 1 + 10, .h = 32 };
  ex.tiles.resize( 2 );
  ex.tiles[0].tile  = e_tile::indentured_servant;
  ex.tiles[0].where = point{ .x = -2, .y = 0 };
  ex.tiles[1].tile  = e_tile::indentured_servant;
  ex.tiles[1].where = point{ .x = -1, .y = 0 };
  REQUIRE( f() == ex );

  // More complex spacing.
  in                                       = {};
  in.source_spec.bounds                    = 20;
  in.source_spec.spread_spec.count         = 6;
  in.source_spec.spread_spec.trimmed.start = 2;
  in.source_spec.spread_spec.trimmed.len   = 10;
  in.progress_spread.spacings.resize( 5 );
  in.progress_spread.spacings[0].mod     = 1;
  in.progress_spread.spacings[0].spacing = 1;
  in.progress_spread.spacings[1].mod     = 3;
  in.progress_spread.spacings[1].spacing = 2;
  in.progress_spread.spacings[2].mod     = 2;
  in.progress_spread.spacings[2].spacing = 3;
  in.progress_spread.spacings[3].mod     = 4;
  in.progress_spread.spacings[3].spacing = 4;
  in.progress_spread.spacings[4].mod     = 6;
  in.progress_spread.spacings[4].spacing = 5;
  in.rendered_count                      = 6;
  in.tile = e_tile::indentured_servant;

  ex               = {};
  ex.bounds.origin = { .x = 0, .y = 0 };
  ex.bounds.size   = { .w = 1 + 4 + 3 + 8 + 1 + 10, .h = 32 };
  ex.tiles.resize( 6 );
  ex.tiles[0].tile  = e_tile::indentured_servant;
  ex.tiles[0].where = point{ .x = -2, .y = 0 };
  ex.tiles[1].tile  = e_tile::indentured_servant;
  ex.tiles[1].where = point{ .x = -2 + 1, .y = 0 };
  ex.tiles[2].tile  = e_tile::indentured_servant;
  ex.tiles[2].where = point{ .x = -2 + 1 + 4, .y = 0 };
  ex.tiles[3].tile  = e_tile::indentured_servant;
  ex.tiles[3].where = point{ .x = -2 + 1 + 4 + 3, .y = 0 };
  ex.tiles[4].tile  = e_tile::indentured_servant;
  ex.tiles[4].where = point{ .x = -2 + 1 + 4 + 3 + 8, .y = 0 };
  ex.tiles[5].tile  = e_tile::indentured_servant;
  ex.tiles[5].where =
      point{ .x = -2 + 1 + 4 + 3 + 8 + 1, .y = 0 };
  REQUIRE( f() == ex );

  // More complex spacing, with label.
  in                                       = {};
  in.source_spec.bounds                    = 20;
  in.source_spec.spread_spec.count         = 6;
  in.source_spec.spread_spec.trimmed.start = 2;
  in.source_spec.spread_spec.trimmed.len   = 10;
  in.progress_spread.spacings.resize( 5 );
  in.progress_spread.spacings[0].mod     = 1;
  in.progress_spread.spacings[0].spacing = 1;
  in.progress_spread.spacings[1].mod     = 3;
  in.progress_spread.spacings[1].spacing = 2;
  in.progress_spread.spacings[2].mod     = 2;
  in.progress_spread.spacings[2].spacing = 3;
  in.progress_spread.spacings[3].mod     = 4;
  in.progress_spread.spacings[3].spacing = 4;
  in.progress_spread.spacings[4].mod     = 6;
  in.progress_spread.spacings[4].spacing = 5;
  in.rendered_count                      = 6;
  in.tile         = e_tile::indentured_servant;
  in.label_policy = SpreadLabels::always{};

  ex               = {};
  ex.bounds.origin = { .x = 0, .y = 0 };
  ex.bounds.size   = { .w = 1 + 4 + 3 + 8 + 1 + 10, .h = 32 };
  ex.tiles.resize( 6 );
  ex.tiles[0].tile  = e_tile::indentured_servant;
  ex.tiles[0].where = point{ .x = -2, .y = 0 };
  ex.tiles[1].tile  = e_tile::indentured_servant;
  ex.tiles[1].where = point{ .x = -2 + 1, .y = 0 };
  ex.tiles[2].tile  = e_tile::indentured_servant;
  ex.tiles[2].where = point{ .x = -2 + 1 + 4, .y = 0 };
  ex.tiles[3].tile  = e_tile::indentured_servant;
  ex.tiles[3].where = point{ .x = -2 + 1 + 4 + 3, .y = 0 };
  ex.tiles[4].tile  = e_tile::indentured_servant;
  ex.tiles[4].where = point{ .x = -2 + 1 + 4 + 3 + 8, .y = 0 };
  ex.tiles[5].tile  = e_tile::indentured_servant;
  ex.tiles[5].where =
      point{ .x = -2 + 1 + 4 + 3 + 8 + 1, .y = 0 };
  ex.labels = { SpreadLabelRenderPlan{
    .options = SpreadLabelOptions{ .placement = nothing },
    .text    = "6",
    .bounds  = { .origin = { .x = 0, .y = 0 },
                 .size   = { .w = 16, .h = 12 } },
  } };
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "6" )
      .returns( size{ .w = 6 * 2, .h = 8 } );
  REQUIRE( f() == ex );

  // Two icons, with auto-decide label, decided no.
  in                                       = {};
  in.source_spec.bounds                    = 20;
  in.source_spec.spread_spec.count         = 2;
  in.source_spec.spread_spec.trimmed.start = 2;
  in.source_spec.spread_spec.trimmed.len   = 10;
  in.progress_spread.spacings.resize( 1 );
  in.progress_spread.spacings[0].mod     = 1;
  in.progress_spread.spacings[0].spacing = 20;
  in.rendered_count                      = 2;
  in.tile         = e_tile::indentured_servant;
  in.label_policy = SpreadLabels::auto_decide{};

  ex               = {};
  ex.bounds.origin = { .x = 0, .y = 0 };
  ex.bounds.size   = { .w = 20 + 10, .h = 32 };
  ex.tiles.resize( 2 );
  ex.tiles[0].tile  = e_tile::indentured_servant;
  ex.tiles[0].where = point{ .x = -2, .y = 0 };
  ex.tiles[1].tile  = e_tile::indentured_servant;
  ex.tiles[1].where = point{ .x = 18, .y = 0 };
  REQUIRE( f() == ex );

  // Two icons, with auto-decide label, decided yes.
  in                                       = {};
  in.source_spec.bounds                    = 20;
  in.source_spec.spread_spec.count         = 2;
  in.source_spec.spread_spec.trimmed.start = 2;
  in.source_spec.spread_spec.trimmed.len   = 10;
  in.progress_spread.spacings.resize( 1 );
  in.progress_spread.spacings[0].mod     = 1;
  in.progress_spread.spacings[0].spacing = 1;
  in.rendered_count                      = 2;
  in.tile         = e_tile::indentured_servant;
  in.label_policy = SpreadLabels::auto_decide{};

  ex               = {};
  ex.bounds.origin = { .x = 0, .y = 0 };
  ex.bounds.size   = { .w = 1 + 10, .h = 32 };
  ex.tiles.resize( 2 );
  ex.tiles[0].tile  = e_tile::indentured_servant;
  ex.tiles[0].where = point{ .x = -2, .y = 0 };
  ex.tiles[1].tile  = e_tile::indentured_servant;
  ex.tiles[1].where = point{ .x = -1, .y = 0 };
  ex.labels         = { SpreadLabelRenderPlan{
            .options = SpreadLabelOptions{ .placement = nothing },
            .text    = "2",
            .bounds  = { .origin = { .x = 0, .y = 0 },
                         .size   = { .w = 16, .h = 12 } },
  } };
  textometer.EXPECT__dimensions_for_line( rr::TextLayout{}, "2" )
      .returns( size{ .w = 6 * 2, .h = 8 } );
  REQUIRE( f() == ex );
}

TEST_CASE( "[spread] replace_first_n_tiles" ) {
  TileSpreadRenderPlans plans;
  TileSpreadRenderPlans ex;

  int n_replace = {};

  e_tile const from = e_tile::commodity_food_20;
  e_tile const to   = e_tile::product_fish_20;

  auto const f = [&] {
    replace_first_n_tiles( plans, n_replace, from, to );
  };

  // Default.
  plans     = {};
  n_replace = {};
  ex        = {};
  f();
  REQUIRE( plans == ex );

  // Single spread, can replace all.
  plans = {};
  plans.plans.resize( 1 );
  plans.plans[0].tiles.resize( 4 );
  plans.plans[0].tiles[0].tile = e_tile::commodity_food_20;
  plans.plans[0].tiles[1].tile = e_tile::commodity_food_20;
  plans.plans[0].tiles[2].tile = e_tile::commodity_food_20;
  plans.plans[0].tiles[3].tile = e_tile::commodity_food_20;
  n_replace                    = 2;

  ex = {};
  ex.plans.resize( 1 );
  ex.plans[0].tiles.resize( 4 );
  ex.plans[0].tiles[0].tile = e_tile::product_fish_20;
  ex.plans[0].tiles[1].tile = e_tile::product_fish_20;
  ex.plans[0].tiles[2].tile = e_tile::commodity_food_20;
  ex.plans[0].tiles[3].tile = e_tile::commodity_food_20;
  f();
  REQUIRE( plans == ex );

  // Single spread, can't replace all.
  plans = {};
  plans.plans.resize( 1 );
  plans.plans[0].tiles.resize( 4 );
  plans.plans[0].tiles[0].tile = e_tile::commodity_food_20;
  plans.plans[0].tiles[1].tile = e_tile::commodity_food_20;
  plans.plans[0].tiles[2].tile = e_tile::commodity_food_20;
  plans.plans[0].tiles[3].tile = e_tile::commodity_food_20;
  n_replace                    = 5;

  ex = {};
  ex.plans.resize( 1 );
  ex.plans[0].tiles.resize( 4 );
  ex.plans[0].tiles[0].tile = e_tile::product_fish_20;
  ex.plans[0].tiles[1].tile = e_tile::product_fish_20;
  ex.plans[0].tiles[2].tile = e_tile::product_fish_20;
  ex.plans[0].tiles[3].tile = e_tile::product_fish_20;
  f();
  REQUIRE( plans == ex );

  // Single spread, skip overlay.
  plans = {};
  plans.plans.resize( 1 );
  plans.plans[0].tiles.resize( 4 );
  plans.plans[0].tiles[0].tile       = e_tile::commodity_food_20;
  plans.plans[0].tiles[0].is_overlay = true;
  plans.plans[0].tiles[1].tile       = e_tile::commodity_food_20;
  plans.plans[0].tiles[2].tile       = e_tile::commodity_food_20;
  plans.plans[0].tiles[3].tile       = e_tile::commodity_food_20;
  n_replace                          = 2;

  ex = {};
  ex.plans.resize( 1 );
  ex.plans[0].tiles.resize( 4 );
  ex.plans[0].tiles[0].tile       = e_tile::commodity_food_20;
  ex.plans[0].tiles[0].is_overlay = true;
  ex.plans[0].tiles[1].tile       = e_tile::product_fish_20;
  ex.plans[0].tiles[2].tile       = e_tile::product_fish_20;
  ex.plans[0].tiles[3].tile       = e_tile::commodity_food_20;
  f();
  REQUIRE( plans == ex );

  // Single spread, stop early.
  plans = {};
  plans.plans.resize( 1 );
  plans.plans[0].tiles.resize( 4 );
  plans.plans[0].tiles[0].tile = e_tile::commodity_food_20;
  plans.plans[0].tiles[1].tile = e_tile::commodity_furs_20;
  plans.plans[0].tiles[2].tile = e_tile::commodity_food_20;
  plans.plans[0].tiles[3].tile = e_tile::commodity_food_20;
  n_replace                    = 3;

  ex = {};
  ex.plans.resize( 1 );
  ex.plans[0].tiles.resize( 4 );
  ex.plans[0].tiles[0].tile = e_tile::product_fish_20;
  ex.plans[0].tiles[1].tile = e_tile::commodity_furs_20;
  ex.plans[0].tiles[2].tile = e_tile::commodity_food_20;
  ex.plans[0].tiles[3].tile = e_tile::commodity_food_20;
  f();
  REQUIRE( plans == ex );

  // Single spread, can replace all, multiple spreads.
  plans = {};
  plans.plans.resize( 2 );
  plans.plans[0].tiles.resize( 4 );
  plans.plans[0].tiles[0].tile = e_tile::commodity_food_20;
  plans.plans[0].tiles[1].tile = e_tile::commodity_food_20;
  plans.plans[0].tiles[2].tile = e_tile::commodity_food_20;
  plans.plans[0].tiles[3].tile = e_tile::commodity_food_20;
  plans.plans[1].tiles.resize( 4 );
  plans.plans[1].tiles[0].tile = e_tile::commodity_food_20;
  plans.plans[1].tiles[1].tile = e_tile::commodity_food_20;
  plans.plans[1].tiles[2].tile = e_tile::commodity_food_20;
  plans.plans[1].tiles[3].tile = e_tile::commodity_food_20;
  n_replace                    = 6;

  ex = {};
  ex.plans.resize( 2 );
  ex.plans[0].tiles.resize( 4 );
  ex.plans[0].tiles[0].tile = e_tile::product_fish_20;
  ex.plans[0].tiles[1].tile = e_tile::product_fish_20;
  ex.plans[0].tiles[2].tile = e_tile::product_fish_20;
  ex.plans[0].tiles[3].tile = e_tile::product_fish_20;
  ex.plans[1].tiles.resize( 4 );
  ex.plans[1].tiles[0].tile = e_tile::product_fish_20;
  ex.plans[1].tiles[1].tile = e_tile::product_fish_20;
  ex.plans[1].tiles[2].tile = e_tile::commodity_food_20;
  ex.plans[1].tiles[3].tile = e_tile::commodity_food_20;
  f();
  REQUIRE( plans == ex );
}

TEST_CASE(
    "[spread] render_plan_for_tile_uncompressed_spread" ) {
  (void)render_plan_for_tile_uncompressed_spread;
}

} // namespace
} // namespace rn
