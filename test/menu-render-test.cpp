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

// Testing
#include "test/mocking.hpp"
#include "test/mocks/render/itextometer.hpp"

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
  rr::MockTextometer textometer;

  textometer.EXPECT__font_height().by_default().returns( 8 );

  string const item_names[] = {
    "Move Units",          //
    "View Mode",           //
    "European Status",     //
    "Find Colony",         //
    "Zoom In",             //
    "Zoom Out",            //
    "Zoom Default",        //
    "Find Blinking Unit",  //
    "Show Hidden Terrain", //
    "Center View",         //
    "Window  ",            //
  };
  for( string const& item_name : item_names )
    textometer
        .EXPECT__dimensions_for_line( rr::TextLayout{},
                                      item_name )
        .returns(
            size{ .w = 6 * int( ssize( item_name ) ), .h = 8 } );

  static_assert( base::Show<MenuRenderLayout> );

  auto f = [&] {
    return build_menu_rendered_layout( textometer, menu, screen,
                                       positions );
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
     .text_layout = { .monospace    = false,
                      .spacing      = nothing,
                      .line_spacing = nothing },
     .position    = { .where       = { .x = 32, .y = 16 },
                      .orientation = e_diagonal_direction::nw,
                      .parent_side = nothing },
     .bounds      = { .origin = { .x = 32, .y = 16 },
                      .size   = { .w = 134, .h = 165 } },
     .items       = { { .item = e_menu_item::move,
                        .text = "Move Units",
                        .bounds_relative =
                            { .origin = { .x = 4, .y = 4 },
                              .size   = { .w = 126, .h = 12 } },
                        .bounds_absolute =
                            { .origin = { .x = 36, .y = 20 },
                              .size   = { .w = 126, .h = 12 } },
                        .text_nw_relative = { .x = 6, .y = 2 },
                        .has_arrow        = false },
                      { .item = e_menu_item::view_mode,
                        .text = "View Mode",
                        .bounds_relative =
                            { .origin = { .x = 4, .y = 16 },
                              .size   = { .w = 126, .h = 12 } },
                        .bounds_absolute =
                            { .origin = { .x = 36, .y = 32 },
                              .size   = { .w = 126, .h = 12 } },
                        .text_nw_relative = { .x = 6, .y = 2 },
                        .has_arrow        = false },
                      { .item = e_menu_item::harbor_view,
                        .text = "European Status",
                        .bounds_relative =
                            { .origin = { .x = 4, .y = 28 },
                              .size   = { .w = 126, .h = 12 } },
                        .bounds_absolute =
                            { .origin = { .x = 36, .y = 44 },
                              .size   = { .w = 126, .h = 12 } },
                        .text_nw_relative = { .x = 6, .y = 2 },
                        .has_arrow        = false },
                      { .item = e_menu_item::find_colony,
                        .text = "Find Colony",
                        .bounds_relative =
                            { .origin = { .x = 4, .y = 45 },
                              .size   = { .w = 126, .h = 12 } },
                        .bounds_absolute =
                            { .origin = { .x = 36, .y = 61 },
                              .size   = { .w = 126, .h = 12 } },
                        .text_nw_relative = { .x = 6, .y = 2 },
                        .has_arrow        = false },
                      { .item = e_menu_item::zoom_in,
                        .text = "Zoom In",
                        .bounds_relative =
                            { .origin = { .x = 4, .y = 62 },
                              .size   = { .w = 126, .h = 12 } },
                        .bounds_absolute =
                            { .origin = { .x = 36, .y = 78 },
                              .size   = { .w = 126, .h = 12 } },
                        .text_nw_relative = { .x = 6, .y = 2 },
                        .has_arrow        = false },
                      { .item = e_menu_item::zoom_out,
                        .text = "Zoom Out",
                        .bounds_relative =
                            { .origin = { .x = 4, .y = 74 },
                              .size   = { .w = 126, .h = 12 } },
                        .bounds_absolute =
                            { .origin = { .x = 36, .y = 90 },
                              .size   = { .w = 126, .h = 12 } },
                        .text_nw_relative = { .x = 6, .y = 2 },
                        .has_arrow        = false },
                      { .item = e_menu_item::restore_zoom,
                        .text = "Zoom Default",
                        .bounds_relative =
                            { .origin = { .x = 4, .y = 86 },
                              .size   = { .w = 126, .h = 12 } },
                        .bounds_absolute =
                            { .origin = { .x = 36, .y = 102 },
                              .size   = { .w = 126, .h = 12 } },
                        .text_nw_relative = { .x = 6, .y = 2 },
                        .has_arrow        = false },
                      { .item = e_menu_item::find_blinking_unit,
                        .text = "Find Blinking Unit",
                        .bounds_relative =
                            { .origin = { .x = 4, .y = 103 },
                              .size   = { .w = 126, .h = 12 } },
                        .bounds_absolute =
                            { .origin = { .x = 36, .y = 119 },
                              .size   = { .w = 126, .h = 12 } },
                        .text_nw_relative = { .x = 6, .y = 2 },
                        .has_arrow        = false },
                      { .item = e_menu_item::hidden_terrain,
                        .text = "Show Hidden Terrain",
                        .bounds_relative =
                            { .origin = { .x = 4, .y = 120 },
                              .size   = { .w = 126, .h = 12 } },
                        .bounds_absolute =
                            { .origin = { .x = 36, .y = 136 },
                              .size   = { .w = 126, .h = 12 } },
                        .text_nw_relative = { .x = 6, .y = 2 },
                        .has_arrow        = false },
                      { .item = e_menu_item::center_view,
                        .text = "Center View",
                        .bounds_relative =
                            { .origin = { .x = 4, .y = 132 },
                              .size   = { .w = 126, .h = 12 } },
                        .bounds_absolute =
                            { .origin = { .x = 36, .y = 148 },
                              .size   = { .w = 126, .h = 12 } },
                        .text_nw_relative = { .x = 6, .y = 2 },
                        .has_arrow        = false },
                      { .item = nothing,
                        .text = "Window  ",
                        .bounds_relative =
                            { .origin = { .x = 4, .y = 149 },
                              .size   = { .w = 126, .h = 12 } },
                        .bounds_absolute =
                            { .origin = { .x = 36, .y = 165 },
                              .size   = { .w = 126, .h = 12 } },
                        .text_nw_relative = { .x = 6, .y = 2 },
                        .has_arrow        = true } },
     .bars        = { { .origin = { .x = 32, .y = 56 },
                        .size   = { .w = 134, .h = 5 } },
                      { .origin = { .x = 32, .y = 73 },
                        .size   = { .w = 134, .h = 5 } },
                      { .origin = { .x = 32, .y = 114 },
                        .size   = { .w = 134, .h = 5 } },
                      { .origin = { .x = 32, .y = 131 },
                        .size   = { .w = 134, .h = 5 } },
                      { .origin = { .x = 32, .y = 160 },
                        .size   = { .w = 134, .h = 5 } } } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[menu-render] build_menu_bar_rendered_layout" ) {
  vector<e_menu> contents;
  MenuBarRenderedLayout expected;
  rect screen;
  rr::MockTextometer textometer;

  string const header_names[] = {
    "Game",  "View",  "Orders", "Reports",   "Trade",
    "Cheat", "Music", "Window", "Land View", "Revolopedia" };
  for( string const& header_name : header_names )
    textometer
        .EXPECT__dimensions_for_line( rr::TextLayout{},
                                      header_name )
        .returns( size{ .w = 6 * int( ssize( header_name ) ),
                        .h = 8 } );

  static_assert( base::Show<MenuBarRenderedLayout> );

  auto f = [&] {
    return build_menu_bar_rendered_layout( textometer, screen,
                                           contents );
  };

  screen = { .origin = {}, .size = { .w = 640, .h = 360 } };
  // These will be in a different order than usual in the game
  // since we are not reading them in order that they appear in
  // the config, but it doesn't really matter for the purpose of
  // this test.
  for( e_menu const menu : enum_values<e_menu> )
    contents.push_back( menu );
  expected = {
    .bounds  = { .origin = { .x = 0, .y = 0 },
                 .size   = { .w = 640, .h = 16 } },
    .headers = {
      { .menu             = e_menu::game,
        .text             = "Game",
        .text_nw_absolute = { .x = 14, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 8, .y = 1 },
                              .size   = { .w = 36, .h = 12 } } },
      { .menu             = e_menu::view,
        .text             = "View",
        .text_nw_absolute = { .x = 58, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 52, .y = 1 },
                              .size   = { .w = 36, .h = 12 } } },
      { .menu             = e_menu::orders,
        .text             = "Orders",
        .text_nw_absolute = { .x = 102, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 96, .y = 1 },
                              .size   = { .w = 48, .h = 12 } } },
      { .menu             = e_menu::reports,
        .text             = "Reports",
        .text_nw_absolute = { .x = 158, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 152, .y = 1 },
                              .size   = { .w = 54, .h = 12 } } },
      { .menu             = e_menu::trade,
        .text             = "Trade",
        .text_nw_absolute = { .x = 220, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 214, .y = 1 },
                              .size   = { .w = 42, .h = 12 } } },
      { .menu             = e_menu::cheat,
        .text             = "Cheat",
        .text_nw_absolute = { .x = 270, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 264, .y = 1 },
                              .size   = { .w = 42, .h = 12 } } },
      { .menu             = e_menu::music,
        .text             = "Music",
        .text_nw_absolute = { .x = 320, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 314, .y = 1 },
                              .size   = { .w = 42, .h = 12 } } },
      { .menu             = e_menu::window,
        .text             = "Window",
        .text_nw_absolute = { .x = 370, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 364, .y = 1 },
                              .size   = { .w = 48, .h = 12 } } },
      { .menu             = e_menu::land_view,
        .text             = "Land View",
        .text_nw_absolute = { .x = 426, .y = 3 },
        .bounds_absolute  = { .origin = { .x = 420, .y = 1 },
                              .size   = { .w = 66, .h = 12 } } },
      { .menu             = e_menu::pedia,
        .text             = "Revolopedia",
        .text_nw_absolute = { .x = 564, .y = 3 },
        .bounds_absolute  = {
           .origin = { .x = 558, .y = 1 },
           .size   = { .w = 78, .h = 12 } } } } };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
