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
#include "aliases.hpp"
#include "geo-types.hpp"

// SDL
#include "SDL.h"

// c++ standard library
#include <optional>
#include <variant>

namespace rn::input {

// When dragging starts, the `start` will be set with the coord
// and will remain constant throughout the dragging. When the
// drag is released then the `finished` will be set to true. In
// that case both the `start` and `finished` will be set, but
// will only remain so for one event; they will be reset to
// nullopt and false on the very next event. When dragging is in
// progress or just finished one should get the current mouse
// position or drag-lift-off position from the usual mouse state
// variables, i.e., there is not a special dragging "end"
// coordinate variable. The way to use it is:
//
// auto dragging = ...dragging_state_t object...;
//
// if( dragging.in_progress ) {
//   /* starting coord = *dragging.in_progress */
//   if( !dragging.finished ) {
//     /* get mouse pos and do animation */
//   } else {
//     /* If the necessary state changes have already been */
//     /* made in the !dragging.finished branch, then we do */
//     /* not need to do anything here. Otherwise... get mouse */
//     /* pos to find ending value and make state changes. */
//   }
// }
struct dragging_state_t {
  Opt<Coord> in_progress{};
  bool       finished{};
};

struct mouse_state_t {
  bool  left   = false;
  bool  middle = false;
  bool  right  = false;
  Coord pos{}; // current mouse position in logical coords
  dragging_state_t left_drag{};
  dragging_state_t right_drag{};
};

enum class e_mouse_button {
  none,
  left_down,
  left_up,
  right_down,
  right_up,
};

enum class e_mouse_event_kind { button, move, wheel };

struct mouse_event_t {
  e_mouse_event_kind kind{};
  e_mouse_button     buttons = e_mouse_button::none;
  // It should always be the case that prev + delta = pos.
  Coord prev{}; // previous mouse position
  Delta delta{};
  // Mouse wheel: positive/negative
  int wheel_delta{};
};

enum class e_key_change { up, down };

struct key_event_t {
  e_key_change   change;
  ::SDL_Keycode  keycode  = ::SDLK_UNKNOWN;
  ::SDL_Scancode scancode = ::SDL_SCANCODE_UNKNOWN;
  // If the key in question happens to correspond to a
  // directional motion then this will be populated. It will be
  // populated both for key-up and key-down. It may not be useful
  // in many cases.
  Opt<e_direction> direction{};
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

Opt<event_t> poll_event();

// This will consume (without processing) all queued input
// events.
void eat_all_events();

} // namespace rn::input
