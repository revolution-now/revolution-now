/****************************************************************
**menu-render-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-25.
*
* Description: Unit tests for the menu-render module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/menu-render.hpp"

// config
#include "src/config/menu-items.rds.hpp"

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

using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;
using ::refl::enum_values;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[menu-render] build_menu_rendered_layout" ) {
  e_menu menu = {};
  MenuAllowedPositions positions;
  MenuRenderLayout expected;
  rect screen;

  static_assert( base::Show<MenuRenderLayout> );

  auto f = [&] {
    return build_menu_rendered_layout( menu, screen, positions );
  };

  menu      = e_menu::view;
  screen    = { .origin = {}, .size = { .w = 640, .h = 360 } };
  positions = { .positions_allowed = {
                  MenuAllowedPosition{
                    .where       = { .x = 32, .y = 16 },
                    .orientation = e_diagonal_direction::ne,
                    .parent_side = {},
                  },
                  MenuAllowedPosition{
                    .where       = { .x = 32, .y = 16 },
                    .orientation = e_diagonal_direction::nw,
                    .parent_side = {},
                  },
                } };
  expected  = MenuRenderLayout{
     .position =
        MenuPosition{ .where       = { .x = 32, .y = 16 },
                       .orientation = e_diagonal_direction::nw,
                       .parent_side = nothing },
     .bounds = { .origin = { .x = 32, .y = 16 },
                 .size   = { .w = 134, .h = 124 } },
     .items  = { MenuItemRenderLayout{
                   .item = e_menu_item::zoom_in,
                   .text = "Zoom In",
                   .bounds_relative =
                       { .origin = { .x = 4, .y = 4 },
                         .size   = { .w = 126, .h = 12 } },
                   .bounds_absolute =
                       { .origin = { .x = 36, .y = 20 },
                         .size   = { .w = 126, .h = 12 } },
                   .text_nw_relative = { .x = 6, .y = 2 },
                   .has_arrow        = false },
                 MenuItemRenderLayout{
                   .item = e_menu_item::zoom_out,
                   .text = "Zoom Out",
                   .bounds_relative =
                       { .origin = { .x = 4, .y = 16 },
                         .size   = { .w = 126, .h = 12 } },
                   .bounds_absolute  = { .origin = { .x = 36,
                                                     .y = 32 },
                                         .size   = { .w = 126,
                                                     .h = 12 } },
                   .text_nw_relative = { .x = 6, .y = 2 },
                   .has_arrow        = false },
                 MenuItemRenderLayout{
                   .item = e_menu_item::restore_zoom,
                   .text = "Zoom Default",
                   .bounds_relative =
                       { .origin = { .x = 4, .y = 28 },
                         .size   = { .w = 126, .h = 12 } },
                   .bounds_absolute =
                       { .origin = { .x = 36, .y = 44 },
                         .size   = { .w = 126, .h = 12 } },
                   .text_nw_relative = { .x = 6, .y = 2 },
                   .has_arrow        = false },
                 MenuItemRenderLayout{
                   .item = e_menu_item::find_blinking_unit,
                   .text = "Find Blinking Unit",
                   .bounds_relative =
                       { .origin = { .x = 4, .y = 45 },
                         .size   = { .w = 126, .h = 12 } },
                   .bounds_absolute =
                       { .origin = { .x = 36, .y = 61 },
                         .size   = { .w = 126, .h = 12 } },
                   .text_nw_relative = { .x = 6, .y = 2 },
                   .has_arrow        = false },
                 MenuItemRenderLayout{
                   .item = e_menu_item::harbor_view,
                   .text = "European Status",
                   .bounds_relative =
                       { .origin = { .x = 4, .y = 62 },
                         .size   = { .w = 126, .h = 12 } },
                   .bounds_absolute =
                       { .origin = { .x = 36, .y = 78 },
                         .size   = { .w = 126, .h = 12 } },
                   .text_nw_relative = { .x = 6, .y = 2 },
                   .has_arrow        = false },
                 MenuItemRenderLayout{
                   .item = e_menu_item::hidden_terrain,
                   .text = "Show Hidden Terrain",
                   .bounds_relative =
                       { .origin = { .x = 4, .y = 79 },
                         .size   = { .w = 126, .h = 12 } },
                   .bounds_absolute =
                       { .origin = { .x = 36, .y = 95 },
                         .size   = { .w = 126, .h = 12 } },
                   .text_nw_relative = { .x = 6, .y = 2 },
                   .has_arrow        = false },
                 MenuItemRenderLayout{
                   .item = e_menu_item::toggle_view_mode,
                   .text = "Toggle View Mode",
                   .bounds_relative =
                       { .origin = { .x = 4, .y = 91 },
                         .size   = { .w = 126, .h = 12 } },
                   .bounds_absolute =
                       { .origin = { .x = 36, .y = 107 },
                         .size   = { .w = 126, .h = 12 } },
                   .text_nw_relative = { .x = 6, .y = 2 },
                   .has_arrow        = false },
                 MenuItemRenderLayout{
                   .item = nothing,
                   .text = "Window  ",
                   .bounds_relative =
                       { .origin = { .x = 4, .y = 108 },
                         .size   = { .w = 126, .h = 12 } },
                   .bounds_absolute =
                       { .origin = { .x = 36, .y = 124 },
                         .size   = { .w = 126, .h = 12 } },
                   .text_nw_relative = { .x = 6, .y = 2 },
                   .has_arrow        = true } },
     .bars   = { { .origin = { .x = 32, .y = 56 },
                   .size   = { .w = 134, .h = 5 } },
                 { .origin = { .x = 32, .y = 73 },
                   .size   = { .w = 134, .h = 5 } },
                 { .origin = { .x = 32, .y = 90 },
                   .size   = { .w = 134, .h = 5 } },
                 { .origin = { .x = 32, .y = 119 },
                   .size   = { .w = 134, .h = 5 } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[menu-render] build_menu_bar_rendered_layout" ) {
  vector<e_menu> contents;
  MenuBarRenderedLayout expected;
  rect screen;

  static_assert( base::Show<MenuBarRenderedLayout> );

  auto f = [&] {
    return build_menu_bar_rendered_layout( screen, contents );
  };

  screen = { .origin = {}, .size = { .w = 640, .h = 360 } };
  // These will be in a different order than usual in the game
  // since we are not reading them in order that they appear in
  // the config, but it doesn't really matter for the purpose of
  // this test.
  for( e_menu const menu : enum_values<e_menu> )
    contents.push_back( menu );
  expected = MenuBarRenderedLayout{
    .bounds  = { .origin = point{ .x = 0, .y = 0 },
                 .size   = { .w = 640, .h = 16 } },
    .headers = {
      MenuHeaderRenderLayout{
        .menu             = e_menu::advisors,
        .text             = "Advisors",
        .text_nw_absolute = { .x = 14, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 8, .y = 1 },
                              .size   = { .w = 60, .h = 12 } } },
      MenuHeaderRenderLayout{
        .menu             = e_menu::cheat,
        .text             = "Cheat",
        .text_nw_absolute = { .x = 74, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 68, .y = 1 },
                              .size   = { .w = 42, .h = 12 } } },
      MenuHeaderRenderLayout{
        .menu             = e_menu::game,
        .text             = "Game",
        .text_nw_absolute = { .x = 116, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 110, .y = 1 },
                              .size   = { .w = 36, .h = 12 } } },
      MenuHeaderRenderLayout{
        .menu             = e_menu::music,
        .text             = "Music",
        .text_nw_absolute = { .x = 152, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 146, .y = 1 },
                              .size   = { .w = 42, .h = 12 } } },
      MenuHeaderRenderLayout{
        .menu             = e_menu::orders,
        .text             = "Orders",
        .text_nw_absolute = { .x = 194, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 188, .y = 1 },
                              .size   = { .w = 48, .h = 12 } } },
      MenuHeaderRenderLayout{
        .menu             = e_menu::view,
        .text             = "View",
        .text_nw_absolute = { .x = 242, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 236, .y = 1 },
                              .size   = { .w = 36, .h = 12 } } },
      MenuHeaderRenderLayout{
        .menu             = e_menu::window,
        .text             = "Window",
        .text_nw_absolute = { .x = 278, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 272, .y = 1 },
                              .size   = { .w = 48, .h = 12 } } },
      MenuHeaderRenderLayout{
        .menu             = e_menu::land_view,
        .text             = "Land View",
        .text_nw_absolute = { .x = 326, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 320, .y = 1 },
                              .size   = { .w = 66, .h = 12 } } },
      MenuHeaderRenderLayout{
        .menu             = e_menu::pedia,
        .text             = "Revolopedia",
        .text_nw_absolute = { .x = 560, .y = 3 },
        .bounds_absolute  = {
           .origin = { .x = 554, .y = 1 },
           .size   = { .w = 78, .h = 12 } } } } };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
