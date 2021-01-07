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
#include "error.hpp"
#include "init.hpp"
#include "plane.hpp"
#include "screen.hpp"

namespace rn::compositor {

namespace {

H g_menu_height{ 16 };
W g_panel_width{ 6 * 32 };

void init_compositor() {}

void cleanup_compositor() {}

REGISTER_INIT_ROUTINE( compositor );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
maybe<Rect> section( e_section section ) {
  maybe<Rect> res;
  auto      screen_rect = main_window_logical_rect();
  auto      screen_size = screen_rect.delta();
  auto      menu_height =
      is_plane_enabled( e_plane::menu ) ? g_menu_height : 0_h;
  switch( section ) {
    case e_section::menu_bar:
      if( is_plane_enabled( e_plane::menu ) )
        res = Rect{ 0_x, 0_y, screen_size.w, menu_height };
      // res is nothing if menus are not visible.
      break;
    case e_section::non_menu_bar:
      res = Rect{ 0_x, 0_y + menu_height, screen_size.w,
                  screen_size.h - menu_height };
      break;
    case e_section::viewport:
      // At standard zoom; i.e., these are fixed with respect to
      // viewport state.
      res = Rect{ 0_x, 0_y + menu_height,
                  screen_size.w - g_panel_width,
                  screen_size.h - menu_height };
      break;
    case e_section::panel:
      res = Rect{ 0_x + screen_size.w - g_panel_width,
                  0_y + menu_height, g_panel_width,
                  screen_size.h - menu_height };
      break;
  }
  // Check invariants before returning.
  if( res.has_value() ) {
    DCHECK( res->right_edge() <= screen_rect.right_edge(),
            "section {} is out of bounds", section );
    DCHECK( res->left_edge() >= screen_rect.left_edge(),
            "section {} is out of bounds", section );
    DCHECK( res->bottom_edge() <= screen_rect.bottom_edge(),
            "section {} is out of bounds", section );
    DCHECK( res->top_edge() >= screen_rect.top_edge(),
            "section {} is out of bounds", section );
  }
  return res;
}

} // namespace rn::compositor
