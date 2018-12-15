/****************************************************************
**input.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-14.
*
* Description: Utilities for user input
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "geo-types.hpp"

// SDL
#include <SDL.h>

// c++ standard library
#include <variant>

namespace rn::input {

struct mouse_state_t {
  bool  left   = false;
  bool  middle = false;
  bool  right  = false;
  Coord pos{};
};

enum class e_mouse_button {
  none,
  left_down,
  left_up,
  right_down,
  right_up,
};

struct mouse_event_t {
  e_mouse_button buttons = e_mouse_button::none;
  Delta          delta{};
};

enum class e_key_change { up, down };

struct key_event_t {
  e_key_change   change;
  ::SDL_Keycode  key      = ::SDLK_UNKNOWN;
  ::SDL_Scancode scancode = ::SDL_SCANCODE_UNKNOWN;
};

struct unknown_event_t {};
struct quit_event_t {};

using EventTypes = std::variant<unknown_event_t, quit_event_t,
                                key_event_t, mouse_event_t>;

struct event_t {
  mouse_state_t mouse_state;
  EventTypes    event;
};

event_t from_SDL( ::SDL_Event sdl_event );

} // namespace rn::input
