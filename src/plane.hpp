/****************************************************************
**plane.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-30.
*
* Description: Basic unit of game interface.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// gfx
#include "gfx/coord.hpp"

/****************************************************************
** Forward Decls.
*****************************************************************/
namespace rr {
struct Renderer;
}

namespace gfx {
enum class e_resolution;
}

namespace rn {
enum class e_input_handled;
enum class e_menu_item;
namespace input {
struct event_t;
struct unknown_event_t;
struct key_event_t;
struct quit_event_t;
struct mouse_wheel_event_t;
struct mouse_move_event_t;
struct mouse_drag_event_t;
struct mouse_button_event_t;
struct win_event_t;
struct resolution_event_t;
struct cheat_event_t;
struct mod_keys;
enum class e_mouse_button;
} // namespace input
} // namespace rn

namespace rn {

/****************************************************************
** IPlane
*****************************************************************/
struct IPlane {
  virtual ~IPlane() = default;

  virtual void draw( rr::Renderer& renderer ) const;

  // Called once per frame.
  virtual void advance_state();

  // ------------------------------------------------------------
  // Input
  // ------------------------------------------------------------
  // The default implementation of this will delegate to the spe-
  // cific handlers below, which is typically what you should
  // override instead of this one.
  [[nodiscard]] e_input_handled virtual input(
      input::event_t const& event );

  [[nodiscard]] virtual e_input_handled on_key(
      input::key_event_t const& event );

  [[nodiscard]] virtual e_input_handled on_wheel(
      input::mouse_wheel_event_t const& event );

  [[nodiscard]] virtual e_input_handled on_mouse_move(
      input::mouse_move_event_t const& event );

  [[nodiscard]] virtual e_input_handled on_mouse_drag(
      input::mouse_drag_event_t const& event );

  [[nodiscard]] virtual e_input_handled on_mouse_button(
      input::mouse_button_event_t const& event );

  [[nodiscard]] virtual e_input_handled on_win_event(
      input::win_event_t const& event );

  [[nodiscard]] virtual e_input_handled on_resolution_event(
      input::resolution_event_t const& event );

  [[nodiscard]] virtual e_input_handled on_cheat_event(
      input::cheat_event_t const& event );

  [[nodiscard]] virtual e_input_handled on_unknown_event(
      input::unknown_event_t const& event );

  [[nodiscard]] virtual e_input_handled on_quit(
      input::quit_event_t const& event );

  // ------------------------------------------------------------
  // Input
  // ------------------------------------------------------------
  // This encodes the result of asking a plane if it can handle a
  // drag event:
  //
  // yes: The plane it will be given all the events for that
  // drag.
  //
  // no: The drag will be offered to another plane.
  //
  // swallow: This will cause the drag not to be sent to any
  // planes at all. This is useful if a plane doesn't want to
  // handle a drag but it would not be appropriate for the drag
  // to be given to other planes.
  //
  // motion: The drag event will be sent to the plane but as
  // monormal use motion events and/or mouse button events.
  enum class e_accept_drag {
    yes,         // send using dedicated plane drag API methods.
    no,          // don't send them; try the next plane down.
    yes_but_raw, // send drag events as "raw" input_t.
    motion,      // send drag events as normal motion events.
    swallow      // don't send them to me or to anyone else.
  };

  // This is to determine if a plane is willing to accept a drag
  // event (and also serves as a notification that a drag event
  // has started). If it returns `yes` then it will immediately
  // receive the initial on_drag() event and then continue to re-
  // ceive all the drag events until the current drag ends.
  ND e_accept_drag virtual can_drag(
      input::e_mouse_button button, Coord origin );

  // For drag events from [first, last). This will only be called
  // if the can_drag returned true at the start of the drag
  // action.
  void virtual on_drag( input::mod_keys const& mod,
                        input::e_mouse_button button,
                        Coord origin, Coord prev,
                        Coord current );

  void virtual on_drag_finished( input::mod_keys const& mod,
                                 input::e_mouse_button button,
                                 Coord origin, Coord end );

  // Returns true if and only if the plane can handle this menu
  // item at this moment. This will only be called if the plane
  // has registered itself as able to handle this menu item in
  // the first place. Will be used to control which menu items
  // are disabled. Note: the reason that this not const is be-
  // cause it allows implementations to use the same function to
  // implement this as for implementing handl_menu_click.
  virtual bool will_handle_menu_click( e_menu_item item );

  // Handle the click. This will only be called if the plane has
  // registered itself as being able to handle this item and has
  // returned true for this item in will_handle_menu_click at
  // least once this frame.
  virtual void handle_menu_click( e_menu_item item );

  // Called whenever the logical resolution changes and hence
  // when things may need to have their layout changed.
  virtual void on_logical_resolution_changed(
      gfx::e_resolution resolution ) = 0;
};

struct NoOpPlane : IPlane {
  void on_logical_resolution_changed(
      gfx::e_resolution ) override {}
};

} // namespace rn
