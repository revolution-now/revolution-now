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
#include "globals.hpp"
#include "logging.hpp"
#include "util.hpp"

// Abseil
#include "absl/container/flat_hash_map.h"

using namespace std;

namespace rn::input {

namespace {

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
// struct which is easier to deal with.
event_t from_SDL( ::SDL_Event sdl_event ) {
  event_t event;

  Coord mouse;
  auto  buttons = ::SDL_GetMouseState( &mouse.x._, &mouse.y._ );

  mouse = mouse + ( -g_drawing_origin );
  mouse.clip( g_drawing_region );
  mouse.x /= g_resolution_scale_factor.sx;
  mouse.y /= g_resolution_scale_factor.sy;

  /*
   *  struct mouse_state_t {
   *    bool  left   = false;
   *    bool  middle = false;
   *    bool  right  = false;
   *    Coord pos{};
   *  };
   *
   *  event.mouse_state =
   *      mouse_state_t{bool( buttons & SDL_BUTTON_LMASK ),
   *                    bool( buttons & SDL_BUTTON_MMASK ),
   *                    bool( buttons & SDL_BUTTON_RMASK ),
   *                    mouse,
   *                    {},
   *                    {}};
   */

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

  if( l_drag ) logger->debug( "l_drag: {}", l_drag->origin );
  if( r_drag ) logger->debug( "r_drag: {}", r_drag->origin );

  return event;
}

Opt<event_t> poll_event() {
  ::SDL_Event event;
  if( ::SDL_PollEvent( &event ) != 0 ) {
    return from_SDL( event );
  }
  return nullopt;
}

void eat_all_events() {
  while( poll_event() ) {}
}

} // namespace rn::input
