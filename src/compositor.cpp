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
#include "screen.hpp"

namespace rn::compositor {

namespace {

H g_menu_height{16};
W g_panel_width{6 * 32};

void check_invariants() {
  auto bounds = main_window_logical_rect();
  // Make sure none of the section go off screen.
  for( auto e : values<e_section> ) {
    auto rect = section( e );
    CHECK( rect.right_edge() <= bounds.right_edge(),
           "section {} is out of bounds", e );
    CHECK( rect.left_edge() >= bounds.left_edge(),
           "section {} is out of bounds", e );
    CHECK( rect.bottom_edge() <= bounds.bottom_edge(),
           "section {} is out of bounds", e );
    CHECK( rect.top_edge() >= bounds.top_edge(),
           "section {} is out of bounds", e );
  }
  // Make sure none of the sections overlap.
  for( auto e1 : values<e_section> )
    for( auto e2 : values<e_section> )
      if( e1 != e2 )
        CHECK( !section( e1 ).overlap_with( section( e2 ) ),
               "section {} overlaps with section", e1, e2 );
}

void init_compositor() { check_invariants(); }

void cleanup_compositor() {}

REGISTER_INIT_ROUTINE( compositor );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
Rect section( e_section section ) {
  Rect res;
  auto screen_size = main_window_logical_size();
  switch( section ) {
    case +e_section::menu_bar:
      res = Rect{0_x, 0_y, screen_size.w, g_menu_height};
      break;
    case +e_section::viewport:
      // At standard zoom; i.e., these are fixed with respect to
      // viewport state.
      res = Rect{0_x, 0_y + g_menu_height,
                 screen_size.w - g_panel_width,
                 screen_size.h - g_menu_height};
      break;
    case +e_section::panel:
      res = Rect{0_x + screen_size.w - g_panel_width,
                 0_y + g_menu_height, g_panel_width,
                 screen_size.h - g_menu_height};
      break;
  }
  return res;
}

} // namespace rn::compositor
