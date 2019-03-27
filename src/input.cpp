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

// These maintain the dragging state.
Opt<drag_state_t> l_drag{};
Opt<drag_state_t> r_drag{};

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
  auto  buttons = ::SDL_GetMouseState( &mouse.x._, &mouse.y._ );

  // mouse.clip( ... );
  mouse.x /= g_resolution_scale_factor.sx;
  mouse.y /= g_resolution_scale_factor.sy;

  if( l_drag && l_drag->phase == +e_drag_phase::begin )
    l_drag->phase = e_drag_phase::in_progress;
  if( r_drag && r_drag->phase == +e_drag_phase::begin )
    r_drag->phase = e_drag_phase::in_progress;

  if( l_drag && l_drag->phase == +e_drag_phase::end )
    l_drag = nullopt;
  if( r_drag && r_drag->phase == +e_drag_phase::end )
    r_drag = nullopt;

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
      auto l_button_down = bool( buttons & SDL_BUTTON_LMASK );
      // auto m_button_down = bool( buttons & SDL_BUTTON_MMASK );
      auto r_button_down = bool( buttons & SDL_BUTTON_RMASK );

      auto update_drag = [&mouse]( auto button, auto& drag ) {
        if( button ) {
          if( !drag )
            drag = drag_state_t{mouse, e_drag_phase::begin};
          else
            CHECK( drag->phase == +e_drag_phase::in_progress );
        } else {
          if( drag ) drag->phase = e_drag_phase::end;
        }
      };

      update_drag( l_button_down, l_drag );
      update_drag( r_button_down, r_drag );

      // Since we're in a mouse motion event, if any mouse button
      // dragging state is active then this is a dragging event.
      if( l_drag || r_drag ) {
        mouse_drag_event_t drag_event;
        drag_event.prev = g_prev_mouse_pos;
        drag_event.pos = g_prev_mouse_pos = mouse;
        if( l_drag ) {
          drag_event.button = e_mouse_button::l;
          drag_event.state  = *l_drag;
        } else {
          drag_event.button = e_mouse_button::r;
          drag_event.state  = *r_drag;
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
      if( sdl_event.button.button == SDL_BUTTON_LEFT ) {
        if( l_drag ) {
          CHECK( l_drag->phase != +e_drag_phase::end );
          l_drag->phase = e_drag_phase::end;
          mouse_drag_event_t drag_event;
          // Here we don't update the previous mouse position be-
          // cause this is not a mouse motion event.
          drag_event.prev   = mouse;
          drag_event.pos    = mouse;
          drag_event.button = e_mouse_button::l;
          drag_event.state  = *l_drag;
          event             = drag_event;
        } else {
          mouse_button_event_t button_event;
          button_event.pos     = mouse;
          button_event.buttons = e_mouse_button_event::left_up;
          event                = button_event;
        }
        break;
      }
      if( sdl_event.button.button == SDL_BUTTON_RIGHT ) {
        if( r_drag ) {
          CHECK( r_drag->phase != +e_drag_phase::end );
          r_drag->phase = e_drag_phase::end;
          mouse_drag_event_t drag_event;
          // Here we don't update the previous mouse position be-
          // cause this is not a mouse motion event.
          drag_event.prev   = mouse;
          drag_event.pos    = mouse;
          drag_event.button = e_mouse_button::r;
          drag_event.state  = *r_drag;
          event             = drag_event;
        } else {
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
  auto keymods = ::SDL_GetModState();

  base->l_alt_down   = ( keymods & ::KMOD_LALT );
  base->r_alt_down   = ( keymods & ::KMOD_RALT );
  base->alt_down     = base->l_alt_down || base->r_alt_down;
  base->l_mouse_down = bool( buttons & SDL_BUTTON_LMASK );
  base->r_mouse_down = bool( buttons & SDL_BUTTON_RMASK );

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
