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

namespace rn::input {

namespace {} // namespace

// Takes an SDL event and converts it to our own event descriptor
// struct which is easier to deal with.
event_t from_SDL( ::SDL_Event sdl_event ) {
  event_t event;

  int  mouse_x, mouse_y;
  auto buttons = ::SDL_GetMouseState( &mouse_x, &mouse_y );
  event.mouse_state =
      mouse_state_t{bool( buttons & SDL_BUTTON_LMASK ),
                    bool( buttons & SDL_BUTTON_MMASK ),
                    bool( buttons & SDL_BUTTON_RMASK ),
                    {Y( mouse_y ), X( mouse_x )}};
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
      mouse_event.delta.w = W( sdl_event.motion.xrel );
      mouse_event.delta.h = H( sdl_event.motion.xrel );
      event.event         = mouse_event;
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
