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

// sdl
#include "sdl/include-sdl-base.hpp" // FIXME: remove

// base
#include "base/adl-tag.hpp"
#include "base/heap-value.hpp"
#include "base/variant.hpp"

// C++ standard library.
#include <queue>

namespace gfx {
struct Resolution;
struct Resolutions;
}

namespace rn {
struct IEngine;
}

namespace rn::input {

/****************************************************************
** Event Base
*****************************************************************/
// All event types must inherit either directly or indirectly
// from this type.
struct event_base_t {
  mod_keys mod      = {};
  bool l_mouse_down = false;
  bool r_mouse_down = false;

  bool operator==( event_base_t const& ) const = default;
};

/****************************************************************
** Misc. Event Types
*****************************************************************/
struct unknown_event_t : public event_base_t {
  bool operator==( unknown_event_t const& ) const = default;
};

struct quit_event_t : public event_base_t {
  bool operator==( quit_event_t const& ) const = default;
};

/****************************************************************
** Mouse
*****************************************************************/
class mouse_event_base_t : public event_base_t {
 public:
  mouse_event_base_t( Coord pos_ )
    : event_base_t{}, pos( pos_ ) {}
  Coord pos;

  bool operator==( mouse_event_base_t const& ) const = default;

 protected:
  mouse_event_base_t() = default;
};

struct mouse_button_event_t : public mouse_event_base_t {
  bool operator==( mouse_button_event_t const& ) const = default;
  e_mouse_button_event buttons{};
};

struct mouse_wheel_event_t : public mouse_event_base_t {
  bool operator==( mouse_wheel_event_t const& ) const = default;
  int wheel_delta{};
};

struct mouse_move_event_t : public mouse_event_base_t {
  bool operator==( mouse_move_event_t const& ) const = default;
  Coord prev{}; // previous mouse position
  auto delta() const { return pos - prev; }
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
[[deprecated]] void set_mouse_position( IEngine& engine,
                                        Coord new_pos );

// Whether or not to show the system cursor when the mouse is
// over the game window. This can be useful e.g. when the mouse
// moves out of the viewport so that the user doesn't feel that
// they've lost their cursor.
void set_show_system_cursor( bool show );

// Takes the viewport rect in physical pixels.
bool should_show_system_cursor( gfx::rect const viewport );

// The mouse position that we store globally (so that it can be
// fetched in code that does not have access to a mouse event)
// will be relative to the current viewport origin. So when that
// changes, we need to update the cached event to prevent the
// cursor from appearing to jump.
void update_mouse_pos_with_viewport_change(
    gfx::Resolution const& old_resolution,
    gfx::Resolution const& new_resolution );

/****************************************************************
** Mouse Dragging
*****************************************************************/
// NOTE: this derives from the move event and not the base event.
struct mouse_drag_event_t : public mouse_move_event_t {
  e_mouse_button button{};
  drag_state_t state{};
};

/****************************************************************
** Keyboard
*****************************************************************/
enum class e_key_change {
  up,
  down
};

struct key_event_t : public event_base_t {
  e_key_change change;
  int keycode  = {};
  int scancode = {};
  // If the key in question happens to correspond to a
  // directional motion then this will be populated. It will be
  // populated both for key-up and key-down. It may not be useful
  // in many cases.
  maybe<e_direction> direction{};

  bool operator==( key_event_t const& ) const = default;
};

/****************************************************************
** Window
*****************************************************************/
enum class e_win_event_type {
  resized,
  other
};

struct win_event_t : public event_base_t {
  e_win_event_type type = {};

  bool operator==( win_event_t const& ) const = default;
};

/****************************************************************
** Resolution
*****************************************************************/
struct resolution_event_t : public event_base_t {
  // Declare these and use heap_value so that we can forward de-
  // clare gfx::Resolution.
  resolution_event_t();
  ~resolution_event_t();
  resolution_event_t( resolution_event_t&& ) noexcept;
  resolution_event_t& operator=( resolution_event_t&& ) noexcept;
  resolution_event_t( resolution_event_t const& );
  resolution_event_t& operator=( resolution_event_t const& );

  bool operator==( resolution_event_t const& ) const;

  base::heap_value<gfx::Resolutions> resolutions;
};

/****************************************************************
** Cheat
*****************************************************************/
// This event is sent when the magic cheat key sequence is de-
// tected. It will be sent each time the sequence is detected,
// although within a single game it is a one-way latch.
struct cheat_event_t : public event_base_t {};

/****************************************************************
** Input Events
*****************************************************************/
// clang-format off
using event_base = base::variant<
  unknown_event_t, // non-relevant events
  quit_event_t,    // signal from OS to quit, e.g. x-out window
  key_event_t,
  mouse_move_event_t,
  mouse_button_event_t,
  mouse_wheel_event_t,
  mouse_drag_event_t,
  win_event_t,
  resolution_event_t,
  cheat_event_t
>;
// clang-format on

static_assert( std::equality_comparable<unknown_event_t> );
static_assert( std::equality_comparable<quit_event_t> );
static_assert( std::equality_comparable<key_event_t> );
static_assert( std::equality_comparable<mouse_move_event_t> );
static_assert( std::equality_comparable<mouse_button_event_t> );
static_assert( std::equality_comparable<mouse_wheel_event_t> );
static_assert( std::equality_comparable<mouse_drag_event_t> );
static_assert( std::equality_comparable<win_event_t> );
static_assert( std::equality_comparable<resolution_event_t> );
static_assert( std::equality_comparable<cheat_event_t> );
static_assert( std::equality_comparable<event_base> );

enum class e_input_event {
  unknown_event,
  quit_event,
  key_event,
  mouse_move_event,
  mouse_button_event,
  mouse_wheel_event,
  mouse_drag_event,
  win_event,
  resolution_event,
  cheat_event,
};

struct event_t : public event_base {
  using event_base::event_base;

  event_base const& as_base() const { return *this; }
  event_base& as_base() { return *this; }

  bool operator==( event_t const& ) const = default;

  using unknown_event      = unknown_event_t;
  using quit_event         = quit_event_t;
  using key_event          = key_event_t;
  using mouse_move_event   = mouse_move_event_t;
  using mouse_button_event = mouse_button_event_t;
  using mouse_wheel_event  = mouse_wheel_event_t;
  using mouse_drag_event   = mouse_drag_event_t;
  using win_event          = win_event_t;
  using resolution_event   = resolution_event_t;
  using cheat_event        = cheat_event_t;

  using e = e_input_event;
};

NOTHROW_MOVE( event_t );

/****************************************************************
** Event queue.
*****************************************************************/
// TODO: temporary until we get a proper interface in the IEngine
// for inputs.
void clear_event_queue();

// Grab all new events and put them into the queue.
void pump_event_queue( IEngine& engine );

// Callers are responsible for popping used events.
std::queue<event_t>& event_queue();

void inject_resolution_event(
    gfx::Resolutions const& resolutions );

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

// Is there a mod key down in addition to any other key?
bool has_mod_key( key_event_t const& event );

mouse_buttons_state get_mouse_buttons_state();

// Make the mouse position contained in `event` (if there is one)
// relative to an origin that is shifted by `delta` from the cur-
// rent origin.
void move_mouse_origin( event_t& event, Delta delta );

[[nodiscard]] event_t mouse_origin_moved_by(
    event_t const& event, Delta delta );

template<typename T>
requires std::is_convertible_v<T, event_t>
[[nodiscard]] T mouse_origin_moved_by( T const& event,
                                       Delta const delta ) {
  event_t new_event = event;
  move_mouse_origin( new_event, delta );
  return new_event.get<T>();
}

maybe<mouse_event_base_t const&> is_mouse_event(
    event_t const& event );
maybe<Coord const&> mouse_position( event_t const& event );

// These are useful if a client of the input events wants to
// treat dragging as normal mouse motion/click events.
maybe<mouse_button_event_t> drag_event_to_mouse_button_event(
    mouse_drag_event_t const& event );
mouse_move_event_t drag_event_to_mouse_motion_event(
    mouse_drag_event_t const& event );

// This is useful during initialization of something that re-
// sponds to mouse-over events in order to make its state consis-
// tent with the current mouse position since otherwise it
// wouldn't get a mouse move event until the user happens to next
// move the mouse.
mouse_move_event_t mouse_move_event_from_curr_pos();

/****************************************************************
** For Testing
*****************************************************************/
void wait_for_q();
bool is_any_key_down();
bool is_q_down();

} // namespace rn::input

namespace base {

template<>
struct variant_to_enum<::rn::input::event_base> {
  using type = ::rn::input::e_input_event;
};

} // namespace base
