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
  auto matcher = scelta::match(
      []( input::unknown_event_t ) { return false; },
      []( input::quit_event_t ) { return false; },
      [&]( input::key_event_t const& e ) { return on_key( e ); },
      [&]( input::mouse_wheel_event_t const& e ) {
        return on_wheel( e );
      },
      [&]( input::mouse_move_event_t const& e ) {
        return on_mouse_move( e );
      },
      [&]( input::mouse_button_event_t const& e ) {
        return on_mouse_button( e );
      },
      []( input::mouse_drag_event_t ) { return false; } );
  return matcher( event );
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

void Object::on_mouse_leave() {}

} // namespace rn::ui
