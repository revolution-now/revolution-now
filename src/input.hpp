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
#include "macros.hpp"
#include "maybe.hpp"

// Rds
#include "input.rds.hpp"

// gfx
#include "gfx/coord.hpp"

// base
#include "base/adl-tag.hpp"
#include "base/variant.hpp"

// SDL
// TODO: get rid of this
#include "sdl.hpp"

// C++ standard library.
#include <queue>

namespace rn::input {

/****************************************************************
** Event Base
*****************************************************************/
// All event types must inherit either directly or indirectly
// from this type.
struct event_base_t {
  mod_keys mod          = {};
  bool     l_mouse_down = false;
  bool     r_mouse_down = false;
};

/****************************************************************
** Misc. Event Types
*****************************************************************/
struct unknown_event_t : public event_base_t {};
struct quit_event_t : public event_base_t {};

/****************************************************************
** Mouse
*****************************************************************/
class mouse_event_base_t : public event_base_t {
 public:
  mouse_event_base_t( Coord pos_ )
    : event_base_t{}, pos( pos_ ) {}
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

// In logical coordinates. Note that this is not actually the
// most current mouse position known to the hardware or runtime,
// it is the mouse position as recorded when the last event was
// polled. There may be mouse events in the queue that move the
// mouse position but have yet to be fetched by the game, and
// those will not be reflected in the position returned here.
// This situation should probably be fine though, perhaps even
// desirable, since that guarantees that this mouse position
// won't be inconsistent with other parts of the game that are
// processing mouse positions by receiving mouse events.
Coord current_mouse_position();

// This will move the mouse to the given position in the window
// in logical screen coordinates (those used by the game). Note
// that it will do so by generating a mouse motion even, so the
// mouse position won't immediately change until that event is
// polled through the usual mechanism.
void set_mouse_position( Coord new_pos );

/****************************************************************
** Mouse Dragging
*****************************************************************/
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
  maybe<e_direction> direction{};
};

/****************************************************************
** Window
*****************************************************************/
enum class e_win_event_type { resized, other };

struct win_event_t : public event_base_t {
  e_win_event_type type;
};

/****************************************************************
** Input Events
*****************************************************************/
// clang-format off
using event_t = base::variant<
  unknown_event_t, // non-relevant events
  quit_event_t,    // signal from OS to quit, e.g. x-out window
  key_event_t,
  mouse_move_event_t,
  mouse_button_event_t,
  mouse_wheel_event_t,
  mouse_drag_event_t,
  win_event_t
>;
// clang-format on
NOTHROW_MOVE( event_t );

enum class e_input_event {
  unknown_event,
  quit_event,
  key_event,
  mouse_move_event,
  mouse_button_event,
  mouse_wheel_event,
  mouse_drag_event,
  win_event
};

// Grab all new events and put them into the queue.
void pump_event_queue();

// Callers are responsible for popping used events.
std::queue<event_t>& event_queue();

/****************************************************************
** Utilities
*****************************************************************/
// Takes into account the shift key which normal key codes or
// scan codes do not. FIXME: this is only a temporary solution
// until a more proper method is employed based on SDL's
// SDL_TextInputEvent events.
maybe<char> ascii_char_for_event( key_event_t const& event );

// Is this event a shift, control, alt, etc.
bool is_mod_key( key_event_t const& event );

// Make the mouse position contained in `event` (if there is one)
// relative to an origin that is shifted by `delta` from the cur-
// rent origin.
[[nodiscard]] event_t move_mouse_origin_by( event_t const& event,
                                            Delta delta );

maybe<mouse_event_base_t const&> is_mouse_event(
    event_t const& event );
maybe<Coord const&> mouse_position( event_t const& event );

// These are useful if a client of the input events wants to
// treat dragging as normal mouse motion/click events.
maybe<mouse_button_event_t> drag_event_to_mouse_button_event(
    mouse_drag_event_t const& event );
mouse_move_event_t drag_event_to_mouse_motion_event(
    mouse_drag_event_t const& event );

/****************************************************************
** For Testing
*****************************************************************/
void wait_for_q();
bool is_any_key_down();
bool is_q_down();

} // namespace rn::input

namespace base {

template<>
struct variant_to_enum<::rn::input::event_t> {
  using type = ::rn::input::e_input_event;
};

} // namespace base
