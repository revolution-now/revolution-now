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
using ::gfx::rect;
using ::refl::enum_count;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[spread] render_plan_for_tile_spread" ) {
  TileSpreadSpecs tile_spreads;
  TileSpreadRenderPlans expected;

  auto f = [&] {
    return render_plan_for_tile_spread( tile_spreads );
  };

  // Test that a zero-count spread with a label requested does
  // not render a label.
  tile_spreads = {
    .spreads =
        { { .icon_spread = { .spec =
                                 SpreadSpec{
                                   .count   = 0,
                                   .trimmed = { .start = 2,
                                                .len   = 28 } },
                             .rendered_count = 0,
                             .spacing        = 1 },
            .tile        = e_tile::dragoon } },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{} };
  expected = {};
  REQUIRE( f() == expected );

  // One spread, one tile, with label.
  tile_spreads = {
    .spreads =
        { { .icon_spread = { .spec =
                                 SpreadSpec{
                                   .count   = 1,
                                   .trimmed = { .start = 2,
                                                .len   = 28 } },
                             .rendered_count = 1,
                             .spacing        = 1 },
            .tile        = e_tile::dragoon } },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{} };
  expected = {
    .bounds = { .w = 28, .h = 32 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 28, .h = 32 } },
         .tiles  = { { .tile  = e_tile::dragoon,
                       .where = { -2, 0 } } },
         .label =
            SpreadLabelRenderPlan{
               .text  = "1",
               .where = { .x = 0, .y = 0 },
            } },
    } };
  REQUIRE( f() == expected );

  // One spread, one tile, with label, non-default label posi-
  // tion.
  tile_spreads = {
    .spreads =
        { { .icon_spread = { .spec =
                                 SpreadSpec{
                                   .count   = 1,
                                   .trimmed = { .start = 2,
                                                .len   = 28 } },
                             .rendered_count = 1,
                             .spacing        = 1 },
            .tile        = e_tile::dragoon,
            .label_opts =
                SpreadLabelOptions{
                  .placement =
                      SpreadLabelPlacement::in_first_tile{
                        .placement = e_cdirection::sw } } } },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{} };
  expected = {
    .bounds = { .w = 28, .h = 32 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 28, .h = 32 } },
         .tiles  = { { .tile  = e_tile::dragoon,
                       .where = { -2, 0 } } },
         .label =
            SpreadLabelRenderPlan{
               .options =
                  SpreadLabelOptions{
                     .placement =
                        SpreadLabelPlacement::in_first_tile{
                           .placement = e_cdirection::sw } },
               .text  = "1",
               .where = { .x = 0, .y = 32 - 8 - 2 },
            } },
    } };
  REQUIRE( f() == expected );

  // Label uses real count when different from rendered_count.
  tile_spreads = {
    .spreads =
        { { .icon_spread = { .spec =
                                 SpreadSpec{
                                   .count   = 2,
                                   .trimmed = { .start = 2,
                                                .len   = 28 } },
                             .rendered_count = 1,
                             .spacing        = 1 },
            .tile        = e_tile::dragoon } },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{} };
  expected = {
    .bounds = { .w = 28, .h = 32 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 28, .h = 32 } },
         .tiles  = { { .tile  = e_tile::dragoon,
                       .where = { -2, 0 } } },
         .label =
            SpreadLabelRenderPlan{
               .text  = "2",
               .where = { .x = 0, .y = 0 },
            } },
    } };
  REQUIRE( f() == expected );

  // Three spreads, middle empty does not emit group spacing.
  tile_spreads = {
    .spreads =
        {
          { .icon_spread = { .spec =
                                 SpreadSpec{
                                   .count   = 1,
                                   .trimmed = { .start = 2,
                                                .len   = 28 } },
                             .rendered_count = 1,
                             .spacing        = 1 },
            .tile        = e_tile::dragoon },
          { .icon_spread = { .spec =
                                 SpreadSpec{
                                   .count   = 0,
                                   .trimmed = { .start = 2,
                                                .len   = 28 } },
                             .rendered_count = 0,
                             .spacing        = 1 },
            .tile        = e_tile::dragoon },
          { .icon_spread = { .spec =
                                 SpreadSpec{
                                   .count   = 2,
                                   .trimmed = { .start = 11,
                                                .len   = 13 } },
                             .rendered_count = 2,
                             .spacing        = 3 },
            .tile        = e_tile::soldier },
        },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{},
  };
  expected = {
    .bounds = { .w = 28 + 1 + 16, .h = 32 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 28, .h = 32 } },
         .tiles =
             {
              { .tile = e_tile::dragoon, .where = { -2, 0 } },
            },
         .label =
            SpreadLabelRenderPlan{
               .text  = "1",
               .where = { .x = 0, .y = 0 },
            } },
      TileSpreadRenderPlan{
         .bounds = { .origin = { .x = 28 + 1 },
                     .size   = { .w = 13 + 3, .h = 32 } },
         .tiles =
             {
              { .tile = e_tile::soldier, .where = { 18, 0 } },
              { .tile = e_tile::soldier, .where = { 21, 0 } },
            },
         .label =
            SpreadLabelRenderPlan{
               .text  = "2",
               .where = { .x = 29, .y = 0 },
            },
      } } };
  REQUIRE( f() == expected );

  // One spread, one tile, with overlay.
  testing_set_trimmed_cache(
      e_tile::red_x_20, rect{ .origin = { .x = 2, .y = 2 },
                              .size   = { .w = 14, .h = 14 } } );
  tile_spreads = {
    .spreads       = { {
            .icon_spread  = { .spec =
                                  SpreadSpec{
                                    .count   = 1,
                                    .trimmed = { .start = 2,
                                                 .len   = 28 } },
                              .rendered_count = 1,
                              .spacing        = 1 },
            .tile         = e_tile::dragoon,
            .overlay_tile = e_tile::red_x_20,
    } },
    .group_spacing = 1,
    .label_policy  = {} };
  expected = {
    .bounds = { .w = 28, .h = 32 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {}, .size = { .w = 28, .h = 32 } },
         .tiles =
             {
              { .tile       = e_tile::dragoon,
                 .where      = { -2, 0 },
                 .is_overlay = false },
              { .tile       = e_tile::red_x_20,
                 .where      = { -2, 6 },
                 .is_overlay = true },
            },
         .label = nothing },
    } };
  REQUIRE( f() == expected );

  // One spread, one tile, with label, with line breaks.
  tile_spreads = {
    .spreads =
        { { .icon_spread = { .spec =
                                 SpreadSpec{
                                   .count   = 10,
                                   .trimmed = { .start = 2,
                                                .len   = 10 } },
                             .rendered_count = 10,
                             .spacing        = 5 },
            .tile        = e_tile::commodity_food_20,
            .label_opts =
                SpreadLabelOptions{
                  .placement =
                      SpreadLabelPlacement::in_first_tile{
                        .placement = e_cdirection::sw } },
            .line_breaks =
                TileSpreadLineBreaks{ .line_width   = 20,
                                      .line_spacing = 6 } } },
    .group_spacing = 1,
    .label_policy  = SpreadLabels::always{} };
  expected = {
    .bounds = { .w = 20, .h = 20 + 3 * 6 },
    .plans  = {
      TileSpreadRenderPlan{
         .bounds = { .origin = {},
                     .size   = { .w = 20, .h = 20 + 3 * 6 } },
         .tiles =
             {
              { .tile  = e_tile::commodity_food_20,
                 .where = { -2, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 3, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 8, 0 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { -2, 6 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 3, 6 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 8, 6 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { -2, 12 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 3, 12 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { 8, 12 } },
              { .tile  = e_tile::commodity_food_20,
                 .where = { -2, 18 } },
            },
         .label =
            SpreadLabelRenderPlan{
               .options =
                  SpreadLabelOptions{
                     .placement =
                        SpreadLabelPlacement::in_first_tile{
                           .placement = e_cdirection::sw } },
               .text  = "10",
               .where = { .x = 0, .y = 20 - 10 },
            } },
    } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[spread] replace_first_n_tiles" ) {
}

} // namespace
} // namespace rn
