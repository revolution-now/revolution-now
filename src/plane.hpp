/****************************************************************
**plane.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-30.
*
* Description: Rendering planes.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "enum.hpp"
#include "fmt-helper.hpp"
#include "input.hpp"
#include "menu.hpp"
#include "sdl-util.hpp"

// base-util
#include "base-util/non-copyable.hpp"

namespace rn {

enum class e_( plane,
               /* values */
               viewport, // land, units, colonies, etc.
               panel,    // the info panel on the right
               image,    // any of the fullscreen pics displayed
               colony,   // colony view
               europe,   // the old world
               effects,  // such as e.g. fading
               menu,     // the menus at the top of screen
               window,   // the windows
               console   // the developer console
);

struct Plane : public util::non_copy_non_move {
  static Plane& get( e_plane plane );

  // Is this plane enabled.  If not, it won't be rendered.
  bool virtual enabled() const = 0;

  // Will rendering this plane cover all pixels?  If so, then
  // planes under it will not be rendered.
  bool virtual covers_screen() const = 0;

  // Draw the plane to the given texture; note that this texture
  // may not need to be referenced explicitly because it will
  // already be set as the default rendering target before this
  // function is called. The texture is initialized with zero
  // alpha everywhere once during initialization, but thereafter
  // will not be touched; i.e., it will only be modified through
  // these draw() function calls. That way, this function can
  // rely on the texture having the same state that it had at the
  // end of the last such call (if that happens to be useful).
  void virtual draw( Texture const& tx ) const = 0;

  // Accept input; returns true/false depending on whether the
  // input was handled or not.  If it was handled (true) then
  // this input will not be given to any further planes.
  ND bool virtual input( input::event_t const& event );

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
  enum class e_accept_drag { yes, no, motion, swallow };

  // This is to determine if a plane is willing to accept a drag
  // event (and also serves as a notification that a drag event
  // has started). If it returns `true` then it will immediately
  // receive the initial on_drag() event and then continue to
  // receive all the drag events until the current drag ends.
  ND e_accept_drag virtual can_drag(
      input::e_mouse_button button, Coord origin );

  // For drag events from [first, last). This will only be called
  // if the can_drag returned true at the start of the drag
  // action.
  void virtual on_drag( input::e_mouse_button button,
                        Coord origin, Coord prev,
                        Coord current );

  void virtual on_drag_finished( input::e_mouse_button button,
                                 Coord origin, Coord end );

  // This handler function does not take the e_menu_item as a pa-
  // rameter to force the planes to supply a unique handler func-
  // tion for each item that it implements. Otherwise a plane
  // might be tempted to supply a "catch-all" handler; that would
  // be error-prone in that it may end up receiving a request to
  // handle an item that it does not actually handle, which would
  // then require a check failure, which we want to avoid.
  using MenuClickHandler = std::function<void()>;

  // Asks the plane if it can handler a particular menu item. If
  // it returns nullopt that means "no." Otherwise it means
  // "yes," and it must return reference to a handler function
  // which will be called when them item is clicked assuming that
  // the menu item is enabled and if there are no higher planes
  // that also handle it. Default implementation returns nullopt.
  virtual OptRef<MenuClickHandler> menu_click_handler(
      e_menu_item item ) const;
};

void draw_all_planes( Texture const& tx = Texture() );

// Returns true if one of the planes handled the input, false
// otherwise. At most one plane will handle the input.
bool send_input_to_planes( input::event_t const& event );

} // namespace rn
