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

namespace rn::ui {

namespace {} // namespace

void Object::advance_state() {}

bool Object::input( input::event_t const& event ) {
  if( disabled() ) return false;
  switch( event.to_enum() ) {
    case input::e_input_event::unknown_event: //
      return false;
    case input::e_input_event::quit_event:    //
      return false;
    case input::e_input_event::win_event: {
      auto& val = event.get<input::win_event_t>();
      return on_win_event( val );
    }
    case input::e_input_event::key_event: {
      auto& val = event.get<input::key_event_t>();
      return on_key( val );
    }
    case input::e_input_event::mouse_wheel_event: {
      auto& val = event.get<input::mouse_wheel_event_t>();
      return on_wheel( val );
    }
    case input::e_input_event::mouse_move_event: {
      auto& val = event.get<input::mouse_move_event_t>();
      return on_mouse_move( val );
    }
    case input::e_input_event::mouse_button_event: {
      auto& val = event.get<input::mouse_button_event_t>();
      return on_mouse_button( val );
    }
    case input::e_input_event::mouse_drag_event: {
      auto& val = event.get<input::mouse_drag_event_t>();
      return on_mouse_drag( val );
    }
  }
}

bool Object::on_key( input::key_event_t const& /*unused*/ ) {
  return false;
}

bool Object::on_wheel(
    input::mouse_wheel_event_t const& /*unused*/ ) {
  return false;
}

bool Object::on_mouse_move(
    input::mouse_move_event_t const& /*unused*/ ) {
  return false;
}

bool Object::on_mouse_drag(
    input::mouse_drag_event_t const& /*unused*/ ) {
  return false;
}

bool Object::on_mouse_button(
    input::mouse_button_event_t const& /*unused*/ ) {
  return false;
}

bool Object::on_win_event(
    input::win_event_t const& /*unused*/ ) {
  return false;
}

void Object::on_mouse_leave( Coord ) {}
void Object::on_mouse_enter( Coord ) {}

} // namespace rn::ui
