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
#include "coord.hpp"
#include "enum.hpp"

// SDL
// TODO: get rid of this
#include "SDL.h"

// c++ standard library
#include <optional>
#include <variant>

namespace rn::input {

/****************************************************************
** Event Base
*****************************************************************/
// All event types must inherit either directly or indirectly
// from this type.
struct event_base_t {
  bool l_alt_down;
  bool r_alt_down;
  bool alt_down; // either alt down
  bool l_mouse_down;
  bool r_mouse_down;
};

/****************************************************************
** Misc. Event Types
*****************************************************************/
struct unknown_event_t : public event_base_t {};
struct quit_event_t : public event_base_t {};

/****************************************************************
** Mouse
*****************************************************************/
enum class e_mouse_button { l, r };

enum class e_mouse_button_event {
  left_down,
  left_up,
  right_down,
  right_up,
};

class mouse_event_base_t : public event_base_t {
public:
  mouse_event_base_t( Coord pos_ ) : pos( pos_ ) {}
  Coord pos;

protected:
  mouse_event_base_t() = default;
};

struct mouse_button_event_t : public mouse_event_base_t {
  e_mouse_button_event buttons{};
};

struct mouse_wheel_event_t : public mouse_event_base_t {
  int wheel_delta{};
};

struct mouse_move_event_t : public mouse_event_base_t {
  Coord prev{}; // previous mouse position
  auto  delta() const { return pos - prev; }
};

/****************************************************************
** Mouse Dragging
*****************************************************************/
enum class e_( drag_phase,
               /* values */
               begin,       // marks the start of a drag
               in_progress, // middle of a drag
               end          // this event marks the end of a drag
);

struct drag_state_t {
  Coord        origin{};
  e_drag_phase phase{};
};

// NOTE: this derives from the move event and not the base event.
struct mouse_drag_event_t : public mouse_move_event_t {
  e_mouse_button button{};
  drag_state_t   state{};
};

/****************************************************************
** Keyboard
*****************************************************************/
enum class e_key_change { up, down };

struct key_event_t : public event_base_t {
  e_key_change   change;
  ::SDL_Keycode  keycode  = ::SDLK_UNKNOWN;
  ::SDL_Scancode scancode = ::SDL_SCANCODE_UNKNOWN;
  // If the key in question happens to correspond to a
  // directional motion then this will be populated. It will be
  // populated both for key-up and key-down. It may not be useful
  // in many cases.
  Opt<e_direction> direction{};
};

/****************************************************************
** Input Events
*****************************************************************/
// clang-format off
using event_t = std::variant<
  unknown_event_t, // non-relevant events
  quit_event_t,    // signal from OS to quit, e.g. x-out window
  key_event_t,
  mouse_move_event_t,
  mouse_button_event_t,
  mouse_wheel_event_t,
  mouse_drag_event_t
>;
// clang-format on

Opt<event_t> poll_event();

// Returns true if there is an event waiting in the queue that is
// relevant to RN.
// FIXME: this function does not seem to work...
ND bool has_event();

// Make the mouse position contained in `event` (if there is one)
// relative to an origin that is shifted by `delta` from the cur-
// rent origin.
event_t move_mouse_origin_by( event_t const& event,
                              Delta          delta );

bool             is_mouse_event( event_t const& event );
Opt<CRef<Coord>> mouse_position( event_t const& event );

// These are useful if a client of the input events wants to
// treat dragging as normal mouse motion/click events.
Opt<mouse_button_event_t> drag_event_to_mouse_button_event(
    mouse_drag_event_t const& event );
mouse_move_event_t drag_event_to_mouse_motion_event(
    mouse_drag_event_t const& event );

// This will consume (without processing) all queued input
// events.
void eat_all_events();

/****************************************************************
** For Testing
*****************************************************************/
void wait_for_q();
bool is_any_key_down();
bool is_q_down();

} // namespace rn::input
