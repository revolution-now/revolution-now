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
#include "flat-queue.hpp"
#include "fmt-helper.hpp"
#include "macros.hpp"

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
struct mod_keys {
  bool l_shf_down;
  bool r_shf_down;
  bool shf_down; // either shift down
  bool l_alt_down;
  bool r_alt_down;
  bool alt_down; // either alt down
  bool l_ctrl_down;
  bool r_ctrl_down;
  bool ctrl_down; // either ctrl down
};

// All event types must inherit either directly or indirectly
// from this type.
struct event_base_t {
  mod_keys mod;
  bool     l_mouse_down;
  bool     r_mouse_down;
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
using event_t = std::variant<
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

// Grab all new events and put them into the queue.
void pump_event_queue();

// Callers are responsible for popping used events.
flat_queue<event_t>& event_queue();

/****************************************************************
** Utilities
*****************************************************************/
// Takes into account the shift key which normal key codes or
// scan codes do not. FIXME: this is only a temporary solution
// until a more proper method is employed based on SDL's
// SDL_TextInputEvent events.
Opt<char> ascii_char_for_event( key_event_t const& event );

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

/****************************************************************
** For Testing
*****************************************************************/
void wait_for_q();
bool is_any_key_down();
bool is_q_down();

} // namespace rn::input

DEFINE_FORMAT_( ::rn::input::mod_keys, "<mod_keys>" );
