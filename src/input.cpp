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
#include "logger.hpp"
#include "screen.hpp"
#include "sdl.hpp"
#include "util.hpp"
#include "variant.hpp"

// config
#include "config/input.rds.hpp"

// gfx
#include "gfx/resolution.rds.hpp"

// base
#include "base/keyval.hpp"
#include "base/lambda.hpp"
#include "base/meta.hpp"

// rds
#include "input-impl.rds.hpp"

// C++ standard library
#include <algorithm>
#include <array>
#include <unordered_map>

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
drag_phase l_drag{ drag_phase::none{} };
drag_phase r_drag{ drag_phase::none{} };

unordered_map<::SDL_Keycode, e_direction> nav_keys{
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
  auto buf   = config_input.mouse.drag_buffer;
  return abs( delta.w ) > buf || abs( delta.h ) > buf;
}

// Should not call this from outside this module; should instead
// use the mod keys delivered with the input events.
mod_keys query_mod_keys( ::Uint8 const* sdl_keyboard_state ) {
  auto keymods = ::SDL_GetModState();
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
  ::SDL_GetMouseState( &mouse.x, &mouse.y );

  gfx::size const viewport_offset =
      get_global_resolution()
          .member( &gfx::Resolution::viewport )
          .member( &gfx::rect::origin )
          .value_or( gfx::point{} )
          .distance_from_origin();
  int const scale_factor = get_global_resolution()
                               .member( &gfx::Resolution::scale )
                               .value_or( 1 );
  mouse -= Delta::from_gfx( viewport_offset );
  mouse.x /= scale_factor;
  mouse.y /= scale_factor;

  if_get( l_drag, drag_phase::dragging, val ) {
    if( val.phase == e_drag_phase::begin )
      val.phase = e_drag_phase::in_progress;
  }
  if_get( r_drag, drag_phase::dragging, val ) {
    if( val.phase == e_drag_phase::begin )
      val.phase = e_drag_phase::in_progress;
  }

  if_get( l_drag, drag_phase::dragging, val ) {
    if( val.phase == e_drag_phase::end )
      l_drag = drag_phase::none{};
  }
  if_get( r_drag, drag_phase::dragging, val ) {
    if( val.phase == e_drag_phase::end )
      r_drag = drag_phase::none{};
  }

  switch( sdl_event.type ) {
    case ::SDL_QUIT:
      event = quit_event_t{};
      break;
    // FIXME: need to handle these to get unicode input.
    // case ::SDL_TEXTINPUT: {
    //  break;
    //}
    // case ::SDL_TEXTEDITING: {
    //  break;
    //}
    case ::SDL_WINDOWEVENT: {
      win_event_t win_event;
      switch( sdl_event.window.event ) {
        case ::SDL_WINDOWEVENT_MAXIMIZED:
        case ::SDL_WINDOWEVENT_RESTORED:
        case ::SDL_WINDOWEVENT_SIZE_CHANGED:
        case ::SDL_WINDOWEVENT_RESIZED:
          win_event.type = e_win_event_type::resized;
          break;
        default:
          win_event.type = e_win_event_type::other;
          break;
      }
      event = win_event;
      break;
    }
    case ::SDL_KEYDOWN: {
      key_event_t key_event;
      key_event.change   = e_key_change::down;
      key_event.keycode  = sdl_event.key.keysym.sym;
      key_event.scancode = sdl_event.key.keysym.scancode;
      key_event.direction =
          base::lookup( nav_keys, sdl_event.key.keysym.sym );
      event = key_event;
      break;
    }
    case ::SDL_KEYUP: {
      key_event_t key_event;
      key_event.change   = e_key_change::up;
      key_event.keycode  = sdl_event.key.keysym.sym;
      key_event.scancode = sdl_event.key.keysym.scancode;
      key_event.direction =
          base::lookup( nav_keys, sdl_event.key.keysym.sym );
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
          switch( drag.to_enum() ) {
            case drag_phase::e::none: {
              drag = drag_phase::maybe{
                /*origin=*/g_prev_mouse_pos };
              break;
            }
            case drag_phase::e::maybe: {
              if_get( drag, drag_phase::maybe, val ) {
                if( is_in_drag_zone( val.origin, mouse ) ) {
                  drag = drag_phase::dragging{
                    /*origin=*/val.origin,
                    /*phase=*/e_drag_phase::begin };
                }
              }
              break;
            }
            case drag_phase::e::dragging: {
              auto& val =
                  drag.template get<drag_phase::dragging>();
              CHECK( val.phase == e_drag_phase::in_progress );
              break;
            }
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
          switch( drag.to_enum() ) {
            case drag_phase::e::none: //
              break;
            case drag_phase::e::maybe: {
              SHOULD_NOT_BE_HERE;
              break;
            }
            case drag_phase::e::dragging: {
              SHOULD_NOT_BE_HERE;
              break;
            }
          }
        }
      };

      update_drag( l_button_down, l_drag );
      update_drag( r_button_down, r_drag );

      auto l_dragging = holds<drag_phase::dragging>( l_drag );
      auto r_dragging = holds<drag_phase::dragging>( r_drag );

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
        if_get( l_drag, drag_phase::dragging, val ) {
          CHECK( val.phase != e_drag_phase::end );
          val.phase = e_drag_phase::end;
          mouse_drag_event_t drag_event;
          // Here we don't update the previous mouse position be-
          // cause this is not a mouse motion event.
          drag_event.prev   = mouse;
          drag_event.pos    = mouse;
          drag_event.button = e_mouse_button::l;
          drag_event.state = drag_state_t{ /*origin=*/val.origin,
                                           /*phase=*/val.phase };
          event            = drag_event;
        } else if( holds<drag_phase::maybe>( l_drag ) ) {
          l_drag = drag_phase::none{};
          mouse_button_event_t button_event;
          button_event.pos     = mouse;
          button_event.buttons = e_mouse_button_event::left_up;
          event                = button_event;
        } else {
          mouse_button_event_t button_event;
          button_event.pos     = mouse;
          button_event.buttons = e_mouse_button_event::left_up;
          event                = button_event;
        }
        break;
      }
      if( sdl_event.button.button == SDL_BUTTON_RIGHT ) {
        if_get( r_drag, drag_phase::dragging, val ) {
          CHECK( val.phase != e_drag_phase::end );
          val.phase = e_drag_phase::end;
          mouse_drag_event_t drag_event;
          // Here we don't update the previous mouse position be-
          // cause this is not a mouse motion event.
          drag_event.prev   = mouse;
          drag_event.pos    = mouse;
          drag_event.button = e_mouse_button::r;
          drag_event.state = drag_state_t{ /*origin=*/val.origin,
                                           /*phase=*/val.phase };
          event            = drag_event;
        } else if( holds<drag_phase::maybe>( r_drag ) ) {
          r_drag = drag_phase::none{};
          mouse_button_event_t button_event;
          button_event.pos     = mouse;
          button_event.buttons = e_mouse_button_event::right_up;
          event                = button_event;
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
      if( sdl_event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED )
        wheel_event.wheel_delta = -wheel_event.wheel_delta;
      event = wheel_event;
      break;
    }
    default:
      event = unknown_event_t{};
  }

  auto const* keyboard_state = ::SDL_GetKeyboardState( nullptr );

  // Populate the keymod state common to all events. We must do
  // this at the end because `event` is a variant and we don't
  // know at the start of this function which alternative will be
  // chosen.
  auto& base = variant_base<event_base_t>( event );

  // TODO: is the keyboard_state as well as the key mod state
  // (obtained inside the query_mod_keys function) both current
  // with this event?
  base.mod = query_mod_keys( keyboard_state );

  base.l_mouse_down = bool( g_mouse_buttons & SDL_BUTTON_LMASK );
  base.r_mouse_down = bool( g_mouse_buttons & SDL_BUTTON_RMASK );

  return event;
}

} // namespace

Coord current_mouse_position() { return g_prev_mouse_pos; }

void set_mouse_position( Coord new_pos ) {
  int const scale_factor = get_global_resolution()
                               .member( &gfx::Resolution::scale )
                               .value_or( 1 );
  new_pos.x *= scale_factor;
  new_pos.y *= scale_factor;
  // Apparently we can use nullptr for the window and it will use
  // the "focused" one, which seems to work for us.
  ::SDL_WarpMouseInWindow( /*window=*/nullptr, new_pos.x,
                           new_pos.y );
}

void set_show_system_cursor( bool const show ) {
  ::SDL_ShowCursor( show ? SDL_ENABLE : SDL_DISABLE );
}

bool should_show_system_cursor( gfx::rect const viewport ) {
  gfx::point mouse_physical;
  // Need to grab the current mouse position here because the one
  // that we have stored will be relative to the viewport origin
  // which may have just changed (e.g. a window resize or logical
  // resolution change) hence the call to this function.
  ::SDL_GetMouseState( &mouse_physical.x, &mouse_physical.y );
  bool const show_system_cursor =
      !mouse_physical.is_inside( viewport );
  return show_system_cursor;
}

void update_mouse_pos_with_viewport_change(
    gfx::Resolution const& old_resolution,
    gfx::Resolution const& new_resolution ) {
  gfx::point p = g_prev_mouse_pos;
  p *= old_resolution.scale;
  p += old_resolution.viewport.origin.distance_from_origin();
  p -= new_resolution.viewport.origin.distance_from_origin();
  p /= new_resolution.scale;
  g_prev_mouse_pos = Coord::from_gfx( p );
}

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

maybe<::SDL_Event> next_sdl_event() {
  ::SDL_PumpEvents();
  ::SDL_Event event;
  if( ::SDL_PollEvent( &event ) != 0 ) return event;
  return nothing;
}

maybe<event_t> next_event() {
  while( auto event = next_sdl_event() ) {
    if( !is_relevant_event_type( event->type ) ) continue;
    return from_SDL( *event );
  }
  return nothing;
}

constexpr int kMaxEventQueueSize = 10000;
std::queue<event_t> g_event_queue;

} // namespace

void pump_event_queue() {
  while( auto event = input::next_event() )
    if( g_event_queue.size() < kMaxEventQueueSize )
      g_event_queue.push( *event );
}

std::queue<event_t>& event_queue() { return g_event_queue; }

// WARNING: this function has not been tested.
// WARNING: this function has not been tested.
void inject_sdl_window_resize_event() {
  ::SDL_Event event{};
  event.type             = ::SDL_WINDOWEVENT;
  event.window.type      = ::SDL_WINDOWEVENT_RESIZED;
  event.window.data1     = 0;
  event.window.timestamp = ::SDL_GetTicks();
  // Apparently we can use nullptr for the window and it will use
  // the "focused" one, which seems to work for us.
  auto const window_id = ::SDL_GetWindowID( /*window=*/nullptr );
  CHECK( window_id );
  event.window.windowID = window_id;
  // Returns 1 on success, 0 if the event was filtered, or a neg-
  // ative error code on failure; call SDL_GetError() for more
  // information. A common reason for error is the event queue
  // being full.
  auto const res = ::SDL_PushEvent( &event );
  // We don't want the event to get filtered.
  CHECK_NEQ( res, 0 );
  // If it failed then we will just print a warning, since per-
  // haps the queue was full.
  if( res < 0 )
    lg.warn( "SDL_PushEvent failed to push a window event." );
}

void inject_resolution_event(
    gfx::SelectedResolution const& resolution ) {
  resolution_event_t event;
  event.resolution = resolution;
  event_queue().push( event );
}

/****************************************************************
** Utilities
*****************************************************************/
maybe<char> ascii_char_for_event( key_event_t const& event ) {
  static unordered_map<char, char> shift_map{
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
  maybe<char> res;
  if( event.keycode < 128 ) {
    char keychar = char( event.keycode );
    res          = keychar;
    if( event.mod.shf_down && shift_map.contains( keychar ) )
      res = shift_map[keychar];
  }
  return res;
}

bool is_mod_key( key_event_t const& event ) {
  switch( event.keycode ) {
    case ::SDLK_LSHIFT:
    case ::SDLK_RSHIFT:
    case ::SDLK_LCTRL:
    case ::SDLK_RCTRL:
    case ::SDLK_LALT:
    case ::SDLK_RALT:
      return true;
    default:
      return false;
  }
}

bool has_mod_key( key_event_t const& event ) {
  return false                    //
         || event.mod.l_shf_down  //
         || event.mod.r_shf_down  //
         || event.mod.shf_down    //
         || event.mod.l_alt_down  //
         || event.mod.r_alt_down  //
         || event.mod.alt_down    //
         || event.mod.l_ctrl_down //
         || event.mod.r_ctrl_down //
         || event.mod.ctrl_down   //
      ;
}

event_t move_mouse_origin_by( event_t const& event,
                              Delta delta ) {
  event_t new_event = event;
  // This serves two purposes: 1) to tell us whether this is a
  // mouse event or not, and 2) to give us the mouse position.

  // First, move the current mouse position.
  apply_to_alternatives_with_base(
      new_event,
      [&]( mouse_event_base_t& e ) { e.pos -= delta; } );

  // Second, if mouse move event, move the previous mouse
  // position.
  apply_to_alternatives_with_base(
      new_event,
      [&]( mouse_move_event_t& e ) { e.prev -= delta; } );

  // Third, if mouse drag event, move the origin coordinate in-
  // side the drag state.
  apply_to_alternatives_with_base( new_event,
                                   [&]( mouse_drag_event_t& e ) {
                                     e.state.origin -= delta;
                                   } );

  return new_event;
}

maybe<mouse_event_base_t const&> is_mouse_event(
    event_t const& event ) {
  // For any events that are mouse events it will return the base
  // type in a maybe, which can be used for boolean checking and
  // getting useful info out of it.
  return apply_to_alternatives_with_base(
      event, nothing,
      []( mouse_event_base_t const& r )
          -> maybe<mouse_event_base_t const&> { return r; } );
}

maybe<Coord const&> mouse_position( event_t const& event ) {
  return apply_to_alternatives_with_base(
      event, nothing,
      []( mouse_event_base_t const& e ) -> maybe<Coord const&> {
        return e.pos;
      } );
}

maybe<mouse_button_event_t> drag_event_to_mouse_button_event(
    mouse_drag_event_t const& event ) {
  // This works because an e_drag_phase::begin event will always
  // have been preceded with a normal mouse-down event, so the
  // only mouse button event there is to extract from a drag
  // event is the mouse-up that happens when the drag finishes.
  if( event.state.phase != e_drag_phase::end ) return nothing;
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
** resolution_event_t
*****************************************************************/
resolution_event_t::resolution_event_t()  = default;
resolution_event_t::~resolution_event_t() = default;
resolution_event_t::resolution_event_t(
    resolution_event_t&& ) noexcept = default;
resolution_event_t& resolution_event_t::operator=(
    resolution_event_t&& ) noexcept = default;
resolution_event_t::resolution_event_t(
    resolution_event_t const& ) = default;
resolution_event_t& resolution_event_t::operator=(
    resolution_event_t const& ) = default;

bool resolution_event_t::operator==(
    resolution_event_t const& ) const = default;

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
  int count         = 0;
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
