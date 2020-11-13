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
#include "config-files.hpp"
#include "logging.hpp"
#include "screen.hpp"
#include "util.hpp"
#include "variant.hpp"

// Rnl
#include "rnl/input.hpp"

// Revolution Now (config)
#include "../config/ucl/input.inl"

// base-util
#include "base-util/keyval.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

// C++ standard library
#include <algorithm>
#include <array>

using namespace std;

namespace rn::input {

namespace {

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
drag_phase_t l_drag{ drag_phase::none{} };
drag_phase_t r_drag{ drag_phase::none{} };

absl::flat_hash_map<::SDL_Keycode, e_direction> nav_keys{
    { ::SDLK_LEFT, e_direction::w },
    { ::SDLK_RIGHT, e_direction::e },
    { ::SDLK_DOWN, e_direction::s },
    { ::SDLK_UP, e_direction::n },
    { ::SDLK_KP_4, e_direction::w },
    { ::SDLK_KP_6, e_direction::e },
    { ::SDLK_KP_2, e_direction::s },
    { ::SDLK_KP_8, e_direction::n },
    { ::SDLK_KP_7, e_direction::nw },
    { ::SDLK_KP_9, e_direction::ne },
    { ::SDLK_KP_1, e_direction::sw },
    { ::SDLK_KP_3, e_direction::se },
};

bool is_in_drag_zone( Coord current, Coord origin ) {
  auto delta = current - origin;
  auto buf   = config_input.controls.drag_buffer;
  return abs( delta.w._ ) > buf || abs( delta.h._ ) > buf;
}

// Should not call this from outside this module; should instead
// use the mod keys delivered with the input events.
mod_keys query_mod_keys( ::Uint8 const* sdl_keyboard_state ) {
  auto     keymods = ::SDL_GetModState();
  mod_keys mod{};

  mod.l_shf_down  = ( keymods & ::KMOD_LSHIFT );
  mod.r_shf_down  = ( keymods & ::KMOD_RSHIFT );
  mod.l_alt_down  = ( keymods & ::KMOD_LALT );
  mod.r_alt_down  = ( keymods & ::KMOD_RALT );
  mod.l_ctrl_down = ( keymods & ::KMOD_LCTRL );
  mod.r_ctrl_down = ( keymods & ::KMOD_RCTRL );

  if( config_input.keyboard.use_capslock_as_ctrl ) {
    // Below does not seem to work here as SDL seems to want to
    // toggle the caps lock with each press.
    // if( keymods & ::KMOD_CAPS )
    if( sdl_keyboard_state[::SDL_SCANCODE_CAPSLOCK] ) //
      mod.l_ctrl_down = true;
  }

  mod.shf_down  = mod.l_shf_down || mod.r_shf_down;
  mod.alt_down  = mod.l_alt_down || mod.r_alt_down;
  mod.ctrl_down = mod.l_ctrl_down || mod.r_ctrl_down;

  return mod;
}

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
    // FIXME: need to handle these to get unicode input.
    // case ::SDL_TEXTINPUT: {
    //  break;
    //}
    // case ::SDL_TEXTEDITING: {
    //  break;
    //}
    case ::SDL_WINDOWEVENT: {
      win_event_t win_event;
      win_event.type = ( sdl_event.window.event ==
                         ::SDL_WINDOWEVENT_SIZE_CHANGED )
                           ? e_win_event_type::resized
                           : e_win_event_type::other;
      event          = win_event;
      break;
    }
    case ::SDL_KEYDOWN: {
      key_event_t key_event;
      key_event.change   = e_key_change::down;
      key_event.keycode  = sdl_event.key.keysym.sym;
      key_event.scancode = sdl_event.key.keysym.scancode;
      key_event.direction =
          bu::val_safe( nav_keys, sdl_event.key.keysym.sym );
      event = key_event;
      break;
    }
    case ::SDL_KEYUP: {
      key_event_t key_event;
      key_event.change   = e_key_change::up;
      key_event.keycode  = sdl_event.key.keysym.sym;
      key_event.scancode = sdl_event.key.keysym.scancode;
      key_event.direction =
          bu::val_safe( nav_keys, sdl_event.key.keysym.sym );
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
          switch_( drag ) {
            case_( drag_phase::none ) {
              drag = drag_phase::maybe{
                  /*origin=*/g_prev_mouse_pos };
            }
            case_( drag_phase::maybe ) {
              if_v( drag, drag_phase::maybe, val ) {
                if( is_in_drag_zone( val->origin, mouse ) ) {
                  drag = drag_phase::dragging{
                      /*origin=*/val->origin,
                      /*phase=*/e_drag_phase::begin };
                }
              }
            }
            case_( drag_phase::dragging ) {
              CHECK( val.phase == +e_drag_phase::in_progress );
            }
            switch_exhaustive;
          }
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
          switch_( drag ) {
            case_( drag_phase::none ) {}
            case_( drag_phase::maybe ) { SHOULD_NOT_BE_HERE; }
            case_( drag_phase::dragging ) { SHOULD_NOT_BE_HERE; }
            switch_exhaustive;
          }
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
              drag_state_t{ /*origin=*/dragging.origin,
                            /*phase=*/dragging.phase };
        } else {
          auto& dragging =
              std::get<drag_phase::dragging>( r_drag );
          drag_event.button = e_mouse_button::r;
          drag_event.state =
              drag_state_t{ /*origin=*/dragging.origin,
                            /*phase=*/dragging.phase };
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
          drag_event.state =
              drag_state_t{ /*origin=*/val->origin,
                            /*phase=*/val->phase };
          event = drag_event;
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
          drag_event.state =
              drag_state_t{ /*origin=*/val->origin,
                            /*phase=*/val->phase };
          event = drag_event;
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

  auto const* keyboard_state = ::SDL_GetKeyboardState( nullptr );

  // Populate the keymod state common to all events. We must do
  // this at the end because `event` is a variant and we don't
  // know at the start of this function which alternative will be
  // chosen.
  auto* base = variant_base_ptr<event_base_t>( event );
  CHECK( base );

  // TODO: is the keyboard_state as well as the key mod state
  // (obtained inside the query_mod_keys function) both current
  // with this event?
  base->mod = query_mod_keys( keyboard_state );

  base->l_mouse_down =
      bool( g_mouse_buttons & SDL_BUTTON_LMASK );
  base->r_mouse_down =
      bool( g_mouse_buttons & SDL_BUTTON_RMASK );

  return event;
}

} // namespace

Coord current_mouse_position() { return g_prev_mouse_pos; }

/****************************************************************
** SDL Event Queue
*****************************************************************/
namespace {

// This function returns the list of events that we care about
// from SDL in this game.
auto const& relevant_sdl_events() {
  static auto constexpr events =
      array<::SDL_EventType, 11>{ ::SDL_QUIT,
                                  ::SDL_APP_TERMINATING,
                                  ::SDL_WINDOWEVENT,
                                  ::SDL_KEYDOWN,
                                  ::SDL_KEYUP,
                                  ::SDL_TEXTEDITING,
                                  ::SDL_TEXTINPUT,
                                  ::SDL_MOUSEMOTION,
                                  ::SDL_MOUSEBUTTONDOWN,
                                  ::SDL_MOUSEBUTTONUP,
                                  ::SDL_MOUSEWHEEL };
  return events;
}

bool is_relevant_event_type( ::SDL_EventType type ) {
  for( auto t : relevant_sdl_events() )
    if( t == type ) return true;
  return false;
}

// Unfortunately SDL_Event::type is a 32 bit int, while
// ::SDL_EventType is an enum.
bool is_relevant_event_type( ::Uint32 type ) {
  return is_relevant_event_type( ::SDL_EventType( type ) );
}

Opt<::SDL_Event> next_sdl_event() {
  ::SDL_PumpEvents();
  ::SDL_Event event;
  if( ::SDL_PollEvent( &event ) != 0 ) return event;
  return nullopt;
}

Opt<event_t> next_event() {
  while( auto event = next_sdl_event() ) {
    if( !is_relevant_event_type( event->type ) ) continue;
    return from_SDL( *event );
  }
  return nullopt;
}

constexpr int       kMaxEventQueueSize = 10000;
flat_queue<event_t> g_event_queue;

} // namespace

void pump_event_queue() {
  while( auto event = input::next_event() )
    if( g_event_queue.size() < kMaxEventQueueSize )
      g_event_queue.push( *event );
}

flat_queue<event_t>& event_queue() { return g_event_queue; }

/****************************************************************
** Utilities
*****************************************************************/
Opt<char> ascii_char_for_event( key_event_t const& event ) {
  static FlatMap<char, char> shift_map{
      { '`', '~' },  { '1', '!' },  { '2', '@' }, { '3', '#' },
      { '4', '$' },  { '5', '%' },  { '6', '^' }, { '7', '&' },
      { '8', '*' },  { '9', '(' },  { '0', ')' }, { '-', '_' },
      { '=', '+' },  { 'q', 'Q' },  { 'w', 'W' }, { 'e', 'E' },
      { 'r', 'R' },  { 't', 'T' },  { 'y', 'Y' }, { 'u', 'U' },
      { 'i', 'I' },  { 'o', 'O' },  { 'p', 'P' }, { '[', '{' },
      { ']', '}' },  { '\\', '|' }, { 'a', 'A' }, { 's', 'S' },
      { 'd', 'D' },  { 'f', 'F' },  { 'g', 'G' }, { 'h', 'H' },
      { 'j', 'J' },  { 'k', 'K' },  { 'l', 'L' }, { ';', ':' },
      { '\'', '"' }, { 'z', 'Z' },  { 'x', 'X' }, { 'c', 'C' },
      { 'v', 'V' },  { 'b', 'B' },  { 'n', 'N' }, { 'm', 'M' },
      { ',', '<' },  { '.', '>' },  { '/', '?' } };
  Opt<char> res;
  if( event.keycode < 128 ) {
    char keychar = char( event.keycode );
    res          = keychar;
    if( event.mod.shf_down && shift_map.contains( keychar ) )
      res = shift_map[keychar];
  }
  return res;
}

event_t move_mouse_origin_by( event_t const& event,
                              Delta          delta ) {
  event_t new_event = event;
  // This serves two purposes: 1) to tell us whether this is a
  // mouse event or not, and 2) to give us the mouse position.

  // First, move the current mouse position.
  auto ptr = matcher_( new_event, ->, Coord* ) {
    case_( unknown_event_t ) return nullptr;
    case_( quit_event_t ) return nullptr;
    case_( win_event_t ) return nullptr;
    case_( key_event_t ) return nullptr;
    case_( mouse_wheel_event_t ) return &val.pos;
    case_( mouse_move_event_t ) return &val.pos;
    case_( mouse_button_event_t ) return &val.pos;
    case_( mouse_drag_event_t ) return &val.pos;
    matcher_exhaustive;
  };
  if( ptr ) ( *ptr ) -= delta;

  // Second, if mouse move event, move the previous mouse
  // position.
  ptr = matcher_( new_event, ->, Coord* ) {
    case_( unknown_event_t ) return nullptr;
    case_( quit_event_t ) return nullptr;
    case_( win_event_t ) return nullptr;
    case_( key_event_t ) return nullptr;
    case_( mouse_wheel_event_t ) return nullptr;
    case_( mouse_move_event_t ) return &val.prev;
    case_( mouse_button_event_t ) return nullptr;
    case_( mouse_drag_event_t ) return nullptr;
    matcher_exhaustive;
  };
  if( ptr ) ( *ptr ) -= delta;
  return new_event;
}

bool is_mouse_event( event_t const& event ) {
  return matcher_( event ) {
    case_( unknown_event_t ) return false;
    case_( quit_event_t ) return false;
    case_( win_event_t ) return false;
    case_( key_event_t ) return false;
    case_( mouse_wheel_event_t ) return true;
    case_( mouse_move_event_t ) return true;
    case_( mouse_button_event_t ) return true;
    case_( mouse_drag_event_t ) return true;
    matcher_exhaustive;
  }
}

Opt<CRef<Coord>> mouse_position( event_t const& event ) {
  return matcher_( event, ->, Opt<CRef<Coord>> ) {
    case_( unknown_event_t ) return nullopt;
    case_( quit_event_t ) return nullopt;
    case_( win_event_t ) return nullopt;
    case_( key_event_t ) return nullopt;
    case_( mouse_wheel_event_t ) return val.pos;
    case_( mouse_move_event_t ) return val.pos;
    case_( mouse_button_event_t ) return val.pos;
    case_( mouse_drag_event_t ) return val.pos;
    matcher_exhaustive;
  };
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

/****************************************************************
** For Testing
*****************************************************************/
void wait_for_q() {
  while( true ) {
    while( auto event = next_sdl_event() ) {
      if( event->type == ::SDL_KEYDOWN &&
          event->key.keysym.sym == ::SDLK_q )
        return;
    }
    ::SDL_Delay( 50 );
  }
}

bool is_any_key_down() {
  // must pump events in order for key state to be populated, al-
  // though we don't care about the event here.
  ::SDL_PumpEvents();
  int         count = 0;
  auto const* state = ::SDL_GetKeyboardState( &count );
  return any_of( state, state + count, L( _ != 0 ) );
}

bool is_q_down() {
  // must pump events in order for key state to be populated, al-
  // though we don't care about the event here.
  ::SDL_PumpEvents();
  auto const* state = ::SDL_GetKeyboardState( nullptr );
  return state[::SDL_SCANCODE_Q];
}

} // namespace rn::input
