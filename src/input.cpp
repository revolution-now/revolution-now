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

namespace rn::input {

namespace {

// This is used to hold the last "measured" mouse position, where
// "measured" means the last time there was a mouse motion event
// that we processed in this module.  Clients cannot see this
// directly, but will get it in the mouse motion event along with
// the current mouse position whenever it changes.
Coord g_mouse{};

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

  event.mouse_state =
      mouse_state_t{bool( buttons & SDL_BUTTON_LMASK ),
                    bool( buttons & SDL_BUTTON_MMASK ),
                    bool( buttons & SDL_BUTTON_RMASK ), mouse};
  mouse_event_t mouse_event;

  key_event_t key_event;

  switch( sdl_event.type ) {
    case ::SDL_QUIT: event.event = quit_event_t{}; break;
    case ::SDL_KEYDOWN:
      key_event.change   = e_key_change::down;
      key_event.key      = sdl_event.key.keysym.sym;
      key_event.scancode = sdl_event.key.keysym.scancode;
      event.event        = key_event;
      break;
    case ::SDL_KEYUP:
      key_event.change   = e_key_change::up;
      key_event.key      = sdl_event.key.keysym.sym;
      key_event.scancode = sdl_event.key.keysym.scancode;
      event.event        = key_event;
      break;
    case ::SDL_MOUSEMOTION:
      mouse_event.prev  = g_mouse;
      mouse_event.delta = mouse - g_mouse;
      // g_mouse_* needs to hold the previous mouse position.
      g_mouse     = mouse;
      event.event = mouse_event;
      break;
    case ::SDL_MOUSEBUTTONDOWN:
      if( sdl_event.button.button == SDL_BUTTON_LEFT ) {
        mouse_event.buttons = e_mouse_button::left_down;
        event.event         = mouse_event;
        break;
      }
      if( sdl_event.button.button == SDL_BUTTON_RIGHT ) {
        mouse_event.buttons = e_mouse_button::right_down;
        event.event         = mouse_event;
        break;
      }
      break;
    case ::SDL_MOUSEBUTTONUP:
      if( sdl_event.button.button == SDL_BUTTON_LEFT ) {
        mouse_event.buttons = e_mouse_button::left_up;
        event.event         = mouse_event;
        break;
      }
      if( sdl_event.button.button == SDL_BUTTON_RIGHT ) {
        mouse_event.buttons = e_mouse_button::right_up;
        event.event         = mouse_event;
        break;
      }
      break;
    default: event.event = unknown_event_t{};
  }
  return event;
}

} // namespace rn::input
