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
#include "variant.hpp"

namespace rn::ui {

namespace {} // namespace

bool Object::input( input::event_t const& event ) {
  return switch_v( event ) {
    case_v( input::unknown_event_t ) { //
      return false;
    }
    case_v( input::quit_event_t ) { //
      return false;
    }
    case_v( input::key_event_t ) { //
      return on_key( val );
    }
    case_v( input::mouse_wheel_event_t ) { //
      return on_wheel( val );
    }
    case_v( input::mouse_move_event_t ) { //
      return on_mouse_move( val );
    }
    case_v( input::mouse_button_event_t ) { //
      return on_mouse_button( val );
    }
    case_v( input::mouse_drag_event_t ) { //
      return false;
    }
    default_v;
  };
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
