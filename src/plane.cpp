/****************************************************************
**plane.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-30.
*
* Description: Basic unit of game interface.
*
*****************************************************************/
#include "plane.hpp"

// Revolution Now
#include "input.hpp"

using namespace std;

namespace rn {

/****************************************************************
** IPlane
*****************************************************************/
e_input_handled IPlane::input(
    input::event_t const& /*unused*/ ) {
  return e_input_handled::no;
}

void IPlane::advance_state() {}

IPlane::e_accept_drag IPlane::can_drag(
    input::e_mouse_button /*unused*/, Coord /*unused*/ ) {
  return e_accept_drag::no;
}

void IPlane::draw( rr::Renderer& ) const {}

void IPlane::on_drag( input::mod_keys const& /*unused*/,
                      input::e_mouse_button /*unused*/,
                      Coord /*unused*/, Coord /*unused*/,
                      Coord /*unused*/ ) {}

void IPlane::on_drag_finished( input::mod_keys const& /*unused*/,
                               input::e_mouse_button /*unused*/,
                               Coord /*unused*/,
                               Coord /*unused*/ ) {}

bool IPlane::will_handle_menu_click( e_menu_item ) {
  return false;
}

void IPlane::handle_menu_click( e_menu_item ) {}

} // namespace rn
