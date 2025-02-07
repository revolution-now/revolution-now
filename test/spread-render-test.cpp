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
}

} // namespace
} // namespace rn
