/****************************************************************
**input.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-14.
*
* Description: Utilities for user input
*
*****************************************************************/
#include "input.hpp"

// Revolution Now
#include "adt.hpp"
#include "config-files.hpp"
#include "logging.hpp"
#include "screen.hpp"
#include "util.hpp"
#include "variant.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <algorithm>
#include <array>

using namespace std;

// This type will keep track of the dragging state of a given
// mouse button. When a drag is initiated is it represented as a
// `maybe` state; in this state the drag event is not yet sent as
// such to the consumers of input events. Instead, it will remain
// in the `maybe` state until the mouse moves a certain number of
// pixels away from the origin (buffer) to allow for a bit of
// noise in mouse motion during normal clicks. When it leaves
// this buffer region (just a few pixels wide) then it will be
// converted to a `dragging` event and will be sent out as such
// with origin equal to the original origin of the `maybe` state.
// This aims to make clicking easier by preventing flaky behavior
// where the user inadvertantly triggers a drag event by acciden-
// tally moving the mouse a bit during a click (that would be bad
// because it would not be sent as a click event).
ADT( rn::input, drag_phase,      //
     ( none ),                   //
     ( maybe,                    //
       ( Coord, origin ) ),      //
     ( dragging,                 //
       ( Coord, origin ),        //
       ( e_drag_phase, phase ) ) //
);

namespace rn::input {

namespace {

// This function returns the list of events that we care about
// from SDL in this game.
auto const& relevant_sdl_events() {
  static auto constexpr events =
      array<::SDL_EventType, 11>{::SDL_QUIT,
                                 ::SDL_APP_TERMINATING,
                                 ::SDL_WINDOWEVENT,
                                 ::SDL_KEYDOWN,
                                 ::SDL_KEYUP,
                                 ::SDL_TEXTEDITING,
                                 ::SDL_TEXTINPUT,
                                 ::SDL_MOUSEMOTION,
                                 ::SDL_MOUSEBUTTONDOWN,
                                 ::SDL_MOUSEBUTTONUP,
                                 ::SDL_MOUSEWHEEL};
  return events;
}

// This is used to hold the last "measured" mouse position, where
// "measured" means the last time there was a mouse motion event
// that we processed in this module.  Clients cannot see this
// directly, but will get it in the mouse motion event along with
// the current mouse position whenever it changes.
Coord g_prev_mouse_pos{};

using mouse_button_state_t =
    decltype( declval<::SDL_Event>().button.state );
mouse_button_state_t g_mouse_buttons{};

// These maintain the dragging state.
drag_phase_t l_drag{drag_phase::none{}};
drag_phase_t r_drag{drag_phase::none{}};

absl::flat_hash_map<::SDL_Keycode, e_direction> nav_keys{
    {::SDLK_LEFT, e_direction::w},
    {::SDLK_RIGHT, e_direction::e},
    {::SDLK_DOWN, e_direction::s},
    {::SDLK_UP, e_direction::n},
    {::SDLK_KP_4, e_direction::w},
    {::SDLK_KP_6, e_direction::e},
    {::SDLK_KP_2, e_direction::s},
    {::SDLK_KP_8, e_direction::n},
    {::SDLK_KP_7, e_direction::nw},
    {::SDLK_KP_9, e_direction::ne},
    {::SDLK_KP_1, e_direction::sw},
    {::SDLK_KP_3, e_direction::se},
};

bool is_in_drag_zone( Coord current, Coord origin ) {
  auto delta = current - origin;
  auto buf   = config_rn.controls.drag_buffer;
  return abs( delta.w._ ) > buf || abs( delta.h._ ) > buf;
}

} // namespace

// Takes an SDL event and converts it to our own event descriptor
// struct which is easier to deal with. This function's output
// actually depends on when it is run (not just on the SDL_Event
// that it is given as input) because it does things like get the
// current mouse position, so the function signature might be a
// bit decieving.
event_t from_SDL( ::SDL_Event sdl_event ) {
  event_t event;

  Coord mouse;
  // This function returns the button state, but we should not
  // use that because it may not be current with this event.
  // FIXME: should instead use mouse coord current with this
  //        event if it starts causing issues.
  ::SDL_GetMouseState( &mouse.x._, &mouse.y._ );

  // mouse.clip( ... );
  mouse.x /= g_resolution_scale_factor.sx;
  mouse.y /= g_resolution_scale_factor.sy;

  if_v( l_drag, drag_phase::dragging, val ) {
    if( val->phase == +e_drag_phase::begin )
      val->phase = e_drag_phase::in_progress;
  }
  if_v( r_drag, drag_phase::dragging, val ) {
    if( val->phase == +e_drag_phase::begin )
      val->phase = e_drag_phase::in_progress;
  }

  if_v( l_drag, drag_phase::dragging, val ) {
    if( val->phase == +e_drag_phase::end )
      l_drag = drag_phase::none{};
  }
  if_v( r_drag, drag_phase::dragging, val ) {
    if( val->phase == +e_drag_phase::end )
      r_drag = drag_phase::none{};
  }

  switch( sdl_event.type ) {
    case ::SDL_QUIT: event = quit_event_t{}; break;
    case ::SDL_KEYDOWN: {
      key_event_t key_event;
      key_event.change   = e_key_change::down;
      key_event.keycode  = sdl_event.key.keysym.sym;
      key_event.scancode = sdl_event.key.keysym.scancode;
      key_event.direction =
          val_safe( nav_keys, sdl_event.key.keysym.sym );
      event = key_event;
      break;
    }
    case ::SDL_KEYUP: {
      key_event_t key_event;
      key_event.change   = e_key_change::up;
      key_event.keycode  = sdl_event.key.keysym.sym;
      key_event.scancode = sdl_event.key.keysym.scancode;
      key_event.direction =
          val_safe( nav_keys, sdl_event.key.keysym.sym );
      event = key_event;
      break;
    }
    case ::SDL_MOUSEMOTION: {
      auto g_mouse_buttons = sdl_event.motion.state;
      auto l_button_down =
          bool( g_mouse_buttons & SDL_BUTTON_LMASK );
      auto r_button_down =
          bool( g_mouse_buttons & SDL_BUTTON_RMASK );

      auto update_drag = [&mouse]( auto button, auto& drag ) {
        if( button ) {
          auto matcher = scelta::match(
              [&]( drag_phase::none ) {
                drag = drag_phase::maybe{
                    /*origin=*/g_prev_mouse_pos};
              },
              [&]( drag_phase::maybe ) {
                if_v( drag, drag_phase::maybe, val ) {
                  if( is_in_drag_zone( val->origin, mouse ) ) {
                    drag = drag_phase::dragging{
                        /*origin=*/val->origin,
                        /*phase=*/e_drag_phase::begin};
                  }
                }
              },
              []( drag_phase::dragging const& val ) {
                CHECK( val.phase == +e_drag_phase::in_progress );
              } );
          matcher( drag );
        } else {
          // If we were initiating a drag (or in a drag) and re-
          // ceive a mouse motion event then there are three pos-
          // sibilities: 1) the mouse button is still down, in
          // which case we would not be here. 2) the mouse button
          // was released and that event was sent before this mo-
          // tion event, in which case we should not be in the
          // `maybe` drag state or the `dragging` state. 3) the
          // mouse button was released but that event is coming
          // after this one, in which case the button should not
          // appear up (since we extracted the button state from
          // this event, so it should be current with it). There-
          // fore, there is no situation where we should have
          // (button up) + (mouse motion) + (`maybe`/`dragging`).
          auto matcher =
              scelta::match( [&]( drag_phase::none ) {},
                             []( drag_phase::maybe const& ) {
                               SHOULD_NOT_BE_HERE;
                             },
                             []( drag_phase::dragging const& ) {
                               SHOULD_NOT_BE_HERE;
                             } );
          matcher( drag );
        }
      };

      update_drag( l_button_down, l_drag );
      update_drag( r_button_down, r_drag );

      auto l_dragging =
          util::holds<drag_phase::dragging>( l_drag );
      auto r_dragging =
          util::holds<drag_phase::dragging>( r_drag );

      // Since we're in a mouse motion event, if any mouse button
      // dragging state is active then this is a dragging event.
      if( l_dragging || r_dragging ) {
        mouse_drag_event_t drag_event;
        drag_event.prev = g_prev_mouse_pos;
        drag_event.pos = g_prev_mouse_pos = mouse;
        if( l_dragging ) {
          auto& dragging =
              std::get<drag_phase::dragging>( l_drag );
          drag_event.button = e_mouse_button::l;
          drag_event.state =
              drag_state_t{/*origin=*/dragging.origin,
                           /*phase=*/dragging.phase};
        } else {
          auto& dragging =
              std::get<drag_phase::dragging>( r_drag );
          drag_event.button = e_mouse_button::r;
          drag_event.state =
              drag_state_t{/*origin=*/dragging.origin,
                           /*phase=*/dragging.phase};
        }
        event = drag_event;
      } else {
        mouse_move_event_t move_event;
        move_event.prev  = g_prev_mouse_pos;
        move_event.pos   = mouse;
        g_prev_mouse_pos = mouse;
        event            = move_event;
      }
      break;
    }
    case ::SDL_MOUSEBUTTONDOWN: {
      g_mouse_buttons = sdl_event.button.state;
      if( sdl_event.button.button == SDL_BUTTON_LEFT ) {
        mouse_button_event_t button_event;
        button_event.pos     = mouse;
        button_event.buttons = e_mouse_button_event::left_down;
        event                = button_event;
        break;
      }
      if( sdl_event.button.button == SDL_BUTTON_RIGHT ) {
        mouse_button_event_t button_event;
        button_event.pos     = mouse;
        button_event.buttons = e_mouse_button_event::right_down;
        event                = button_event;
        break;
      }
      break;
    }
    case ::SDL_MOUSEBUTTONUP: {
      g_mouse_buttons = sdl_event.button.state;
      if( sdl_event.button.button == SDL_BUTTON_LEFT ) {
        if_v( l_drag, drag_phase::dragging, val ) {
          CHECK( val->phase != +e_drag_phase::end );
          val->phase = e_drag_phase::end;
          mouse_drag_event_t drag_event;
          // Here we don't update the previous mouse position be-
          // cause this is not a mouse motion event.
          drag_event.prev   = mouse;
          drag_event.pos    = mouse;
          drag_event.button = e_mouse_button::l;
          drag_event.state = drag_state_t{/*origin=*/val->origin,
                                          /*phase=*/val->phase};
          event            = drag_event;
        }
        else if( util::holds<drag_phase::maybe>( l_drag ) ) {
          l_drag = drag_phase::none{};
          mouse_button_event_t button_event;
          button_event.pos     = mouse;
          button_event.buttons = e_mouse_button_event::left_up;
          event                = button_event;
        }
        else {
          mouse_button_event_t button_event;
          button_event.pos     = mouse;
          button_event.buttons = e_mouse_button_event::left_up;
          event                = button_event;
        }
        break;
      }
      if( sdl_event.button.button == SDL_BUTTON_RIGHT ) {
        if_v( r_drag, drag_phase::dragging, val ) {
          CHECK( val->phase != +e_drag_phase::end );
          val->phase = e_drag_phase::end;
          mouse_drag_event_t drag_event;
          // Here we don't update the previous mouse position be-
          // cause this is not a mouse motion event.
          drag_event.prev   = mouse;
          drag_event.pos    = mouse;
          drag_event.button = e_mouse_button::r;
          drag_event.state = drag_state_t{/*origin=*/val->origin,
                                          /*phase=*/val->phase};
          event            = drag_event;
        }
        else if( util::holds<drag_phase::maybe>( r_drag ) ) {
          r_drag = drag_phase::none{};
          mouse_button_event_t button_event;
          button_event.pos     = mouse;
          button_event.buttons = e_mouse_button_event::right_up;
          event                = button_event;
        }
        else {
          mouse_button_event_t button_event;
          button_event.pos     = mouse;
          button_event.buttons = e_mouse_button_event::right_up;
          event                = button_event;
        }
        break;
      }
      break;
    }
    case ::SDL_MOUSEWHEEL: {
      mouse_wheel_event_t wheel_event;
      wheel_event.pos = mouse;
      wheel_event.wheel_delta =
          static_cast<int>( sdl_event.wheel.y );
      event = wheel_event;
      break;
    }
    default: event = unknown_event_t{};
  }

  // Populate the keymod state common to all events. We must do
  // this at the end because `event` is a variant and we don't
  // know at the start of this function which alternative will be
  // chosen.
  auto* base = variant_base_ptr<event_base_t>( event );
  CHECK( base );
  // FIXME: need to use key state that is current with this event
  //        being processed if it starts causing issues.
  auto keymods = ::SDL_GetModState();

  base->l_alt_down = ( keymods & ::KMOD_LALT );
  base->r_alt_down = ( keymods & ::KMOD_RALT );
  base->alt_down   = base->l_alt_down || base->r_alt_down;
  base->l_mouse_down =
      bool( g_mouse_buttons & SDL_BUTTON_LMASK );
  base->r_mouse_down =
      bool( g_mouse_buttons & SDL_BUTTON_RMASK );

  return event;
}

// FIXME: ::SDL_HasEvent does not seem to work...
bool has_event() {
  for( auto event_type : relevant_sdl_events() )
    if( ::SDL_HasEvent( event_type ) ) return true;
  return false;
}

event_t move_mouse_origin_by( event_t const& event,
                              Delta          delta ) {
  input::event_t new_event = event;
  // This serves two purposes: 1) to tell us whether this is a
  // mouse event or not, and 2) to give us the mouse position.
  auto matcher = scelta::match(
      []( input::unknown_event_t ) { return (Coord*)nullptr; },
      []( input::quit_event_t ) { return (Coord*)nullptr; },
      []( input::key_event_t ) { return (Coord*)nullptr; },
      []( input::mouse_wheel_event_t& e ) { return &e.pos; },
      []( input::mouse_move_event_t& e ) { return &e.pos; },
      []( input::mouse_button_event_t& e ) { return &e.pos; },
      []( input::mouse_drag_event_t& e ) { return &e.pos; } );
  if( auto mouse_pos_ptr = matcher( new_event ); mouse_pos_ptr )
    ( *mouse_pos_ptr ) -= delta;
  return new_event;
}

bool is_mouse_event( event_t const& event ) {
  auto matcher = scelta::match(
      []( input::unknown_event_t ) { return false; },
      []( input::quit_event_t ) { return false; },
      []( input::key_event_t const& ) { return false; },
      []( input::mouse_wheel_event_t const& ) { return true; },
      []( input::mouse_move_event_t const& ) { return true; },
      []( input::mouse_button_event_t const& ) { return true; },
      []( input::mouse_drag_event_t const& ) { return true; } );
  return matcher( event );
}

Opt<CRef<Coord>> mouse_position( event_t const& event ) {
  auto matcher = scelta::match(
      []( input::unknown_event_t ) {
        return (Coord const*)nullptr;
      },
      []( input::quit_event_t ) {
        return (Coord const*)nullptr;
      },
      []( input::key_event_t ) { return (Coord const*)nullptr; },
      []( input::mouse_wheel_event_t const& e ) {
        return &e.pos;
      },
      []( input::mouse_move_event_t const& e ) {
        return &e.pos;
      },
      []( input::mouse_button_event_t const& e ) {
        return &e.pos;
      },
      []( input::mouse_drag_event_t const& e ) {
        return &e.pos;
      } );
  auto const* res = matcher( event );
  return res ? Opt<CRef<Coord>>{*res} : nullopt;
}

Opt<mouse_button_event_t> drag_event_to_mouse_button_event(
    mouse_drag_event_t const& event ) {
  // This works because an e_drag_phase::begin event will always
  // have been preceded with a normal mouse-down event, so the
  // only mouse button event there is to extract from a drag
  // event is the mouse-up that happens when the drag finishes.
  if( event.state.phase != e_drag_phase::end ) return nullopt;
  mouse_button_event_t res;
  // Copy the base object.
  copy_common_base_object<mouse_event_base_t>( /*from=*/event,
                                               /*to=*/res );
  switch( event.button ) {
    case e_mouse_button::l:
      res.buttons = e_mouse_button_event::left_up;
      break;
    case e_mouse_button::r:
      res.buttons = e_mouse_button_event::right_up;
      break;
  }
  return res;
}

mouse_move_event_t drag_event_to_mouse_motion_event(
    mouse_drag_event_t const& event ) {
  mouse_move_event_t res;
  // Copy the base object.
  copy_common_base_object<mouse_move_event_t>( /*from=*/event,
                                               /*to=*/res );
  return res;
}

Opt<event_t> poll_event() {
  ::SDL_Event event;
  if( ::SDL_PollEvent( &event ) != 0 ) return from_SDL( event );
  return nullopt;
}

void eat_all_events() {
  while( poll_event() ) {}
}

/****************************************************************
** For Testing
*****************************************************************/
void wait_for_q() {
  bool        running = true;
  ::SDL_Event event;
  while( running ) {
    while( ::SDL_PollEvent( &event ) != 0 ) {
      if( event.type == SDL_KEYDOWN &&
          event.key.keysym.sym == ::SDLK_q )
        running = false;
    }
    ::SDL_Delay( 50 );
  }
}

bool is_any_key_down() {
  // must poll events in order for key state to be populated,
  // although we don't care about the event here.
  poll_event();
  int         count = 0;
  auto const* state = SDL_GetKeyboardState( &count );
  return any_of( state, state + count, L( _ != 0 ) );
}

} // namespace rn::input
