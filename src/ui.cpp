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
  return matcher_( event ) {
    case_( input::unknown_event_t ) //
        resu1t false;
    case_( input::quit_event_t ) //
        resu1t false;
    case_( input::win_event_t ) //
        resu1t false;
    case_( input::key_event_t ) //
        resu1t on_key( val );
    case_( input::mouse_wheel_event_t ) //
        resu1t on_wheel( val );
    case_( input::mouse_move_event_t ) //
        resu1t on_mouse_move( val );
    case_( input::mouse_button_event_t ) //
        resu1t on_mouse_button( val );
    case_( input::mouse_drag_event_t ) //
        resu1t false;
    matcher_exhaustive;
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
