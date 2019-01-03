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
Coord g_mouse{};

// When dragging starts, the `start` will be set with the coord
// and will remain constant throughout the dragging. When the
// drag is released then the `finished` will be set to true. In
// that case both the `start` and `finished` will be set, but
// will only remain so for one event; they will be reset to
// nullopt and false on the very next event. When dragging is in
// progress or just finished one should get the current mouse
// position or drag-lift-off position from the usual mouse state
// variables, i.e., there is not a special dragging "end"
// coordinate variable.
Opt<Coord> left_dragging_start{};
bool       left_dragging_finished{false};
Opt<Coord> right_dragging_start{};
bool       right_dragging_finished{false};

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

  event.mouse_state =
      mouse_state_t{bool( buttons & SDL_BUTTON_LMASK ),
                    bool( buttons & SDL_BUTTON_MMASK ),
                    bool( buttons & SDL_BUTTON_RMASK ), mouse};
  mouse_event_t mouse_event;

  key_event_t key_event;

  if( left_dragging_finished ) left_dragging_start = nullopt;
  if( right_dragging_finished ) right_dragging_start = nullopt;
  left_dragging_finished  = false;
  right_dragging_finished = false;

  switch( sdl_event.type ) {
    case ::SDL_QUIT: event.event = quit_event_t{}; break;
    case ::SDL_KEYDOWN:
      key_event.change   = e_key_change::down;
      key_event.keycode  = sdl_event.key.keysym.sym;
      key_event.scancode = sdl_event.key.keysym.scancode;
      key_event.direction =
          val_safe( nav_keys, sdl_event.key.keysym.sym );
      event.event = key_event;
      break;
    case ::SDL_KEYUP:
      key_event.change   = e_key_change::up;
      key_event.keycode  = sdl_event.key.keysym.sym;
      key_event.scancode = sdl_event.key.keysym.scancode;
      key_event.direction =
          val_safe( nav_keys, sdl_event.key.keysym.sym );
      event.event = key_event;
      break;
    case ::SDL_MOUSEMOTION:
      mouse_event.kind  = e_mouse_event_kind::move;
      mouse_event.prev  = g_mouse;
      mouse_event.delta = mouse - g_mouse;
      // g_mouse_* needs to hold the previous mouse position.
      g_mouse     = mouse;
      event.event = mouse_event;

      // Update mouse dragging state.
      if( event.mouse_state.left && !left_dragging_start ) {
        left_dragging_start    = mouse;
        left_dragging_finished = false;
      }
      if( !event.mouse_state.left && left_dragging_start )
        left_dragging_finished = true;
      if( event.mouse_state.right && !right_dragging_start ) {
        right_dragging_start    = mouse;
        right_dragging_finished = false;
      }
      if( !event.mouse_state.right && right_dragging_start )
        right_dragging_finished = true;

      break;
    case ::SDL_MOUSEBUTTONDOWN:
      if( sdl_event.button.button == SDL_BUTTON_LEFT ) {
        mouse_event.kind    = e_mouse_event_kind::button;
        mouse_event.buttons = e_mouse_button::left_down;
        event.event         = mouse_event;
        break;
      }
      if( sdl_event.button.button == SDL_BUTTON_RIGHT ) {
        mouse_event.kind    = e_mouse_event_kind::button;
        mouse_event.buttons = e_mouse_button::right_down;
        event.event         = mouse_event;
        break;
      }
      break;
    case ::SDL_MOUSEBUTTONUP:
      if( sdl_event.button.button == SDL_BUTTON_LEFT ) {
        mouse_event.kind    = e_mouse_event_kind::button;
        mouse_event.buttons = e_mouse_button::left_up;
        event.event         = mouse_event;
        if( left_dragging_start ) left_dragging_finished = true;
        break;
      }
      if( sdl_event.button.button == SDL_BUTTON_RIGHT ) {
        mouse_event.kind    = e_mouse_event_kind::button;
        mouse_event.buttons = e_mouse_button::right_up;
        event.event         = mouse_event;
        if( right_dragging_start )
          right_dragging_finished = true;
        break;
      }
      break;
    case ::SDL_MOUSEWHEEL:
      mouse_event.kind = e_mouse_event_kind::wheel;
      mouse_event.wheel_delta =
          static_cast<int>( sdl_event.wheel.y );
      event.event = mouse_event;
      break;
    default: event.event = unknown_event_t{};
  }

  // Update mouse dragging state.
  event.mouse_state.left_drag.in_progress = left_dragging_start;
  event.mouse_state.right_drag.in_progress =
      right_dragging_start;
  event.mouse_state.left_drag.finished = left_dragging_finished;
  event.mouse_state.right_drag.finished =
      right_dragging_finished;

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
