/****************************************************************
**compositor.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-07-04.
*
* Description: Coordinates layout of elements on screen.
*
*****************************************************************/
#include "compositor.hpp"

// Revolution Now
#include "errors.hpp"
#include "init.hpp"
#include "plane.hpp"
#include "screen.hpp"

namespace rn::compositor {

namespace {

H g_menu_height{ 16 };
W g_panel_width{ 6 * 32 };

void check_invariants() {
  auto bounds = main_window_logical_rect();
  // Make sure none of the section go off screen.
  for( auto e : values<e_section> ) {
    auto maybe_rect = section( e );
    if( !maybe_rect.has_value() ) continue;
    auto const& rect = *maybe_rect;
    CHECK( rect.right_edge() <= bounds.right_edge(),
           "section {} is out of bounds", e );
    CHECK( rect.left_edge() >= bounds.left_edge(),
           "section {} is out of bounds", e );
    CHECK( rect.bottom_edge() <= bounds.bottom_edge(),
           "section {} is out of bounds", e );
    CHECK( rect.top_edge() >= bounds.top_edge(),
           "section {} is out of bounds", e );
  }
}

void init_compositor() { check_invariants(); }

void cleanup_compositor() {}

REGISTER_INIT_ROUTINE( compositor );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
Opt<Rect> section( e_section section ) {
  Opt<Rect> res;
  auto      screen_size = main_window_logical_size();
  auto      menu_height =
      is_plane_enabled( e_plane::menu ) ? g_menu_height : 0_h;
  switch( section ) {
    case +e_section::menu_bar:
      if( is_plane_enabled( e_plane::menu ) )
        res = Rect{ 0_x, 0_y, screen_size.w, menu_height };
      // res is nullopt if menus are not visible.
      break;
    case +e_section::non_menu_bar:
      res = Rect{ 0_x, 0_y + menu_height, screen_size.w,
                  screen_size.h - menu_height };
      break;
    case +e_section::viewport:
      // At standard zoom; i.e., these are fixed with respect to
      // viewport state.
      res = Rect{ 0_x, 0_y + menu_height,
                  screen_size.w - g_panel_width,
                  screen_size.h - menu_height };
      break;
    case +e_section::panel:
      res = Rect{ 0_x + screen_size.w - g_panel_width,
                  0_y + menu_height, g_panel_width,
                  screen_size.h - menu_height };
      break;
  }
  return res;
}

} // namespace rn::compositor
