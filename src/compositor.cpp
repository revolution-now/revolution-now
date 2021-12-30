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
#include "config-files.hpp"
#include "error.hpp"
#include "init.hpp"
#include "lua.hpp"
#include "plane.hpp"
#include "screen.hpp"

// Revolution Now (config)
#include "../config/rcl/ui.inl"

// luapp
#include "luapp/state.hpp"

namespace rn::compositor {

namespace {

W g_panel_width{ 4 * 32 };

H g_console_height = 0_h;

// Is the console at the top or bottom of the screen.
bool g_console_top = false;

void init_compositor() {}

void cleanup_compositor() {}

REGISTER_INIT_ROUTINE( compositor );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void set_console_height( H height ) {
  g_console_height = height;
}

maybe<Rect> section_inverted( e_section sec ) {
  Rect const  screen       = main_window_logical_rect();
  maybe<Rect> non_inverted = section( sec );
  if( !non_inverted ) return screen;

  // Claim: the rectangle can be inverted if and only if there is
  // precisely one edge that is not on the screen edge.
  int not_on_screen_edge =
      ( non_inverted->left_edge() > screen.left_edge() ) +
      ( non_inverted->right_edge() < screen.right_edge() ) +
      ( non_inverted->top_edge() > screen.top_edge() ) +
      ( non_inverted->bottom_edge() < screen.bottom_edge() );
  bool can_be_inverted = ( not_on_screen_edge == 1 );
  if( !can_be_inverted ) return nothing;

  Rect res = *non_inverted;
  if( non_inverted->left_edge() > 0_x )
    return res.with_new_left_edge( 0_x ).with_new_right_edge(
        non_inverted->left_edge() );
  if( non_inverted->right_edge() < screen.right_edge() )
    return res.with_new_right_edge( screen.right_edge() )
        .with_new_left_edge( non_inverted->right_edge() );
  if( non_inverted->top_edge() > 0_y )
    return res.with_new_top_edge( 0_y ).with_new_bottom_edge(
        non_inverted->top_edge() );
  if( non_inverted->bottom_edge() < screen.bottom_edge() )
    return res.with_new_bottom_edge( screen.bottom_edge() )
        .with_new_top_edge( non_inverted->bottom_edge() );
  SHOULD_NOT_BE_HERE;
}

// FIXME: the results of this need to be cached and only recom-
// puted when some state changes that would affect it. This is
// because this function is called multiple times per frame.
// Also, when the compositor layout does change, it should send
// out a notification (an input window resize event) that can be
// picked up by the planes so that they don't have to use
// advance_state to do that.
maybe<Rect> section( e_section sec ) {
  maybe<Rect> res;
  auto        menu_height = is_plane_enabled( e_plane::menu )
                                ? config_ui.menus.menu_bar_height
                                : 0_h;
  switch( sec ) {
    case e_section::menu_bar: {
      if( !is_plane_enabled( e_plane::menu ) ) break;
      UNWRAP_CHECK( normal, section( e_section::normal ) );
      res    = normal;
      res->h = menu_height;
      break;
    }
    case e_section::viewport: {
      // At standard zoom; i.e., these are fixed with respect to
      // viewport state.
      UNWRAP_CHECK( normal, section( e_section::normal ) );
      res = normal;
      res->y += menu_height;
      res->h -= menu_height;
      res->w -= g_panel_width;
      break;
    }
    case e_section::panel: {
      UNWRAP_CHECK( normal, section( e_section::normal ) );
      res = normal;
      res->y += menu_height;
      res->h -= menu_height;
      res->x += res->w - g_panel_width;
      res->w = g_panel_width;
      break;
    }
    case e_section::console: {
      if( g_console_height == 0_h ) break;
      UNWRAP_CHECK( total, section( e_section::total ) );
      if( g_console_top )
        res =
            Rect{ 0_x, 0_y, total.delta().w, g_console_height };
      else
        res = Rect{ 0_x, total.bottom_edge() - g_console_height,
                    total.delta().w, g_console_height };
      break;
    }
    case e_section::normal:
      res = section_inverted( e_section::console );
      break;
    case e_section::total: {
      res = main_window_logical_rect();
      break;
    }
  }
  // Check invariants before returning.
  if( res.has_value() ) {
    Rect total = main_window_logical_rect();
    DCHECK( res->right_edge() <= total.right_edge(),
            "section {} is out of bounds", sec );
    DCHECK( res->left_edge() >= total.left_edge(),
            "section {} is out of bounds", sec );
    DCHECK( res->bottom_edge() <= total.bottom_edge(),
            "section {} is out of bounds", sec );
    DCHECK( res->top_edge() >= total.top_edge(),
            "section {} is out of bounds", sec );
  }
  return res;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( switch_console_position, void ) {
  g_console_top = !g_console_top;
};

}

} // namespace rn::compositor
