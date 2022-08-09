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

// config
#include "config/ui.rds.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

namespace rn::compositor {

namespace {

W g_panel_width{ 4 * 32 };

// Is the console at the top or bottom of the screen.
e_composite_location g_console_loc =
    e_composite_location::bottom;
// Percent of screen dimension occupied.
double g_console_size = 0.0;

void init_compositor() {
  g_console_loc = config_ui.console.default_location;
}

void cleanup_compositor() {}

REGISTER_INIT_ROUTINE( compositor );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void set_console_size( double percent ) {
  g_console_size = percent;
}

void rotate_console() {
  g_console_loc = static_cast<e_composite_location>(
      ( static_cast<int>( g_console_loc ) + 1 ) %
      refl::enum_count<e_composite_location> );
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
  if( non_inverted->left_edge() > 0 )
    return res.with_new_left_edge( 0 ).with_new_right_edge(
        non_inverted->left_edge() );
  if( non_inverted->right_edge() < screen.right_edge() )
    return res.with_new_right_edge( screen.right_edge() )
        .with_new_left_edge( non_inverted->right_edge() );
  if( non_inverted->top_edge() > 0 )
    return res.with_new_top_edge( 0 ).with_new_bottom_edge(
        non_inverted->top_edge() );
  if( non_inverted->bottom_edge() < screen.bottom_edge() )
    return res.with_new_bottom_edge( screen.bottom_edge() )
        .with_new_top_edge( non_inverted->bottom_edge() );
  SHOULD_NOT_BE_HERE;
}

// FIXME
namespace {
bool is_menu_plane_enabled() { return true; }
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
  auto        menu_height = is_menu_plane_enabled()
                                ? config_ui.menus.menu_bar_height
                                : 0;
  switch( sec ) {
    case e_section::menu_bar: {
      if( !is_menu_plane_enabled() ) break;
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
      if( g_console_size == 0.0 ) break;
      UNWRAP_CHECK( total, section( e_section::total ) );
      res = total;
      switch( g_console_loc ) {
        case e_composite_location::top: //
          res->h = static_cast<int>( res->h * g_console_size );
          break;
        case e_composite_location::bottom:
          res->h = static_cast<int>( res->h * g_console_size );
          res->y = total.bottom_edge() - res->h;
          break;
        case e_composite_location::left: //
          res->w = static_cast<int>( res->w * g_console_size );
          break;
        case e_composite_location::right:
          res->w = static_cast<int>( res->w * g_console_size );
          res->x = total.right_edge() - res->w;
          break;
      }
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
    Rect total [[maybe_unused]] = main_window_logical_rect();
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
} // namespace rn::compositor
