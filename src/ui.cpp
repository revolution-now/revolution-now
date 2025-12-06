/****************************************************************
**ui.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-03-16.
*
* Description: Fundamentals for UI.
*
*****************************************************************/
#include "ui.hpp"

// Revolution Now
#include "input.hpp"

// rds
#include "rds/switch-macro.hpp"

namespace rn::ui {

namespace {} // namespace

void object::advance_state() {}

bool object::input( input::event_t const& event ) {
  if( disabled() ) return false;
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
    CASE( cheat_event ) { //
      return on_cheat_event( cheat_event );
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

bool object::on_key( input::key_event_t const& /*unused*/ ) {
  return false;
}

bool object::on_wheel(
    input::mouse_wheel_event_t const& /*unused*/ ) {
  return false;
}

bool object::on_mouse_move(
    input::mouse_move_event_t const& /*unused*/ ) {
  return false;
}

bool object::on_mouse_drag(
    input::mouse_drag_event_t const& /*unused*/ ) {
  return false;
}

bool object::on_mouse_button(
    input::mouse_button_event_t const& /*unused*/ ) {
  return false;
}

bool object::on_win_event(
    input::win_event_t const& /*unused*/ ) {
  return false;
}

bool object::on_resolution_event(
    input::resolution_event_t const& /*unused*/ ) {
  return false;
}

bool object::on_cheat_event(
    input::cheat_event_t const& /*unused*/ ) {
  return false;
}

bool object::on_unknown_event(
    input::unknown_event_t const& /*unused*/ ) {
  return false;
}

bool object::on_quit( input::quit_event_t const& /*unused*/ ) {
  return false;
}

void object::on_mouse_leave( Coord ) {}

void object::on_mouse_enter( Coord ) {}

} // namespace rn::ui
