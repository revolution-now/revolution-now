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

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

/****************************************************************
** IPlane
*****************************************************************/
e_input_handled IPlane::input( input::event_t const& event ) {
  SWITCH( event ) {
    CASE( unknown_event ) { //
      return on_unknown_event( unknown_event );
    }
    CASE( quit_event ) { //
      return on_quit( quit_event );
    }
    CASE( win_event ) { //
      return on_win_event( win_event );
    }
    CASE( resolution_event ) { //
      return on_resolution_event( resolution_event );
    }
    CASE( key_event ) { //
      return on_key( key_event );
    }
    CASE( mouse_move_event ) { //
      return on_mouse_move( mouse_move_event );
    }
    CASE( mouse_button_event ) { //
      return on_mouse_button( mouse_button_event );
    }
    CASE( mouse_wheel_event ) { //
      return on_wheel( mouse_wheel_event );
    }
    CASE( mouse_drag_event ) { //
      return on_mouse_drag( mouse_drag_event );
    }
  }
}

e_input_handled IPlane::on_key( input::key_event_t const& ) {
  return e_input_handled::no;
}

e_input_handled IPlane::on_wheel(
    input::mouse_wheel_event_t const& ) {
  return e_input_handled::no;
}

e_input_handled IPlane::on_mouse_move(
    input::mouse_move_event_t const& ) {
  return e_input_handled::no;
}

e_input_handled IPlane::on_mouse_drag(
    input::mouse_drag_event_t const& ) {
  return e_input_handled::no;
}

e_input_handled IPlane::on_mouse_button(
    input::mouse_button_event_t const& ) {
  return e_input_handled::no;
}

e_input_handled IPlane::on_win_event(
    input::win_event_t const& ) {
  return e_input_handled::no;
}

e_input_handled IPlane::on_resolution_event(
    input::resolution_event_t const& ) {
  return e_input_handled::no;
}

e_input_handled IPlane::on_unknown_event(
    input::unknown_event_t const& ) {
  return e_input_handled::no;
}

e_input_handled IPlane::on_quit( input::quit_event_t const& ) {
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
