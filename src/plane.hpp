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

// Revolution Now
#include "input.hpp"

// Rds
#include "menu.rds.hpp"

namespace rr {
struct Renderer;
}

namespace rn {

/****************************************************************
** Plane
*****************************************************************/
struct Plane {
  virtual ~Plane() = default;

  // Will rendering this plane cover all pixels?  If so, then
  // planes under it will not be rendered.
  bool virtual covers_screen() const = 0;

  void virtual draw( rr::Renderer& renderer ) const = 0;

  // Called once per frame.
  virtual void advance_state();

  // yes:     Will not be given to any other planes.
  // no:      Will try the next plane.

  // Accept input; returns true/false depending on whether the
  // input was handled or not.  If it was handled (true) then
  // this input will not be given to any further planes.
  ND e_input_handled virtual input(
      input::event_t const& event );

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
                        input::e_mouse_button  button,
                        Coord origin, Coord prev,
                        Coord current );

  void virtual on_drag_finished( input::mod_keys const& mod,
                                 input::e_mouse_button  button,
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
};

} // namespace rn
