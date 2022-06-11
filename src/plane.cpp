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

using namespace std;

namespace rn {

/****************************************************************
** Plane
*****************************************************************/
e_input_handled Plane::input(
    input::event_t const& /*unused*/ ) {
  return e_input_handled::no;
}

void Plane::advance_state() {}

Plane::e_accept_drag Plane::can_drag(
    input::e_mouse_button /*unused*/, Coord /*unused*/ ) {
  return e_accept_drag::no;
}

void Plane::draw( rr::Renderer& ) const {}

void Plane::on_drag( input::mod_keys const& /*unused*/,
                     input::e_mouse_button /*unused*/,
                     Coord /*unused*/, Coord /*unused*/,
                     Coord /*unused*/ ) {}

void Plane::on_drag_finished( input::mod_keys const& /*unused*/,
                              input::e_mouse_button /*unused*/,
                              Coord /*unused*/,
                              Coord /*unused*/ ) {}

bool Plane::will_handle_menu_click( e_menu_item ) {
  return false;
}

void Plane::handle_menu_click( e_menu_item ) {}

} // namespace rn
