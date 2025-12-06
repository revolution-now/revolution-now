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

void object::advance_state() {}

bool object::on_up_click(
    input::mouse_button_event_t const& event,
    e_mouse_button_click_state& state ) {
  using enum e_mouse_button_click_state;
  switch( state ) {
    case waiting_for_down:
      return false;
    case waiting_for_up_or_down:
      state = waiting_for_down;
      return on_input( event );
  }
}

bool object::on_down_click(
    input::mouse_button_event_t const& event,
    e_mouse_button_click_state& state ) {
  using enum e_mouse_button_click_state;
  bool const handled = on_input( event );
  // NOTE to derived views: If you don't handle the down click
  // then you don't get the up click.
  if( handled ) state = waiting_for_up_or_down;
  return handled;
}

bool object::input( input::event_t const& event ) {
  SWITCH( event ) {
    // We handle mouse clicks specially since the idea is that we
    // don't ever want to send an up-click to an entity unless
    // that entity has already handled the corresponding down
    // click. This is essential because it allows us to take ac-
    // tions on down clicks that e.g. cause windows or planes to
    // close without worrying that the subsequent up-click will
    // be given to another window/widget/plane, cause a reaction
    // there where it should not.
    CASE( mouse_button_event ) {
      auto const& e = mouse_button_event;
      using enum e_mouse_button_click_state;
      using enum input::e_mouse_button_event;
      switch( e.buttons ) {
        case left_up:
          return on_up_click( e, l_button_state_ );
        case left_down:
          return on_down_click( e, l_button_state_ );
        case right_up:
          return on_up_click( e, r_button_state_ );
        case right_down:
          return on_down_click( e, r_button_state_ );
      }
    }
    default:
      break;
  }
  return on_input( event );
}

bool object::on_input( input::event_t const& event ) {
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
