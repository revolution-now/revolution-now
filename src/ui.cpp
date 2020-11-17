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

bool Object::input( input::event_t const& event ) {
  switch( enum_for( event ) ) {
    case input::e_input_event::unknown_event: //
      return false;
    case input::e_input_event::quit_event: //
      return false;
    case input::e_input_event::win_event: //
      return false;
    case input::e_input_event::key_event: {
      auto& val = get_if_or_die<input::key_event_t>( event );
      return on_key( val );
    }
    case input::e_input_event::mouse_wheel_event: {
      auto& val =
          get_if_or_die<input::mouse_wheel_event_t>( event );
      return on_wheel( val );
    }
    case input::e_input_event::mouse_move_event: {
      auto& val =
          get_if_or_die<input::mouse_move_event_t>( event );
      return on_mouse_move( val );
    }
    case input::e_input_event::mouse_button_event: {
      auto& val =
          get_if_or_die<input::mouse_button_event_t>( event );
      return on_mouse_button( val );
    }
    case input::e_input_event::mouse_drag_event: //
      return false;
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

bool Object::on_mouse_button(
    input::mouse_button_event_t const& /*unused*/ ) {
  return false;
}

void Object::on_mouse_leave( Coord ) {}
void Object::on_mouse_enter( Coord ) {}

} // namespace rn::ui
