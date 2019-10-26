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
#include "tx.hpp"

// base-util
#include "base-util/non-copyable.hpp"

// function_ref
#include "tl/function_ref.hpp"

namespace rn {

enum class e_( plane,
               /* values */
               viewport,  // land, units, colonies, etc.
               panel,     // the info panel on the right
               image,     // any of the fullscreen pics displayed
               colony,    // colony view
               europe,    // the old world
               effects,   // such as e.g. fading
               main_menu, // main menu.
               console,   // the developer console
               menu,      // the menus at the top of screen
               window,    // the windows
               /*******/  //
               omni // Always present, invisible, and first.
);

struct Plane : public util::non_copy_non_move {
  virtual ~Plane() = default;

  static Plane& get( e_plane plane );

  // Will be called on all planes (whether enabled or not) before
  // any other methods are called on it. Default does nothing.
  void virtual initialize();

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
  void virtual draw( Texture& tx ) const = 0;

  // This will be called at the start/end of each frame to allow
  // a plane to update any state that it has. It will only be
  // called on frames that are enabled and visible.
  virtual void on_frame_start();
  virtual void on_frame_end();

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

  // This allows the plane to further customize the drag process
  // if it accepts it. TODO: this should be an ADH.
  struct DragInfo {
    DragInfo( e_accept_drag accept_ ) : accept( accept_ ) {}
    e_accept_drag accept{};
    // This field is relevant if accept == yes.
    Opt<Delta> projection{};
  };
  NOTHROW_MOVE( DragInfo );

  // This is to determine if a plane is willing to accept a drag
  // event (and also serves as a notification that a drag event
  // has started). If it returns `yes` then it will immediately
  // receive the initial on_drag() event and then continue to re-
  // ceive all the drag events until the current drag ends.
  ND DragInfo virtual can_drag( input::e_mouse_button button,
                                Coord                 origin );

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

  // This handler function does not take the e_menu_item as a pa-
  // rameter to force the planes to supply a unique handler func-
  // tion for each item that it implements. Otherwise a plane
  // might be tempted to supply a "catch-all" handler; that would
  // be error-prone in that it may end up receiving a request to
  // handle an item that it does not actually handle, which would
  // then require a check failure, which we want to avoid.
  using MenuClickHandler = tl::function_ref<void()>;

  // Asks the plane if it can handler a particular menu item. If
  // it returns nullopt that means "no." Otherwise it means
  // "yes," and it must return reference to a handler function
  // which will be called when them item is clicked assuming that
  // the menu item is enabled and if there are no higher planes
  // that also handle it. Default implementation returns nullopt.
  //
  // IMPORTANT: Being that this is returning a function_ref, it
  // is important that function returned outlive the function
  // call. So e.g. returning a non-static lambda with captures
  // would probably not be good.
  virtual Opt<MenuClickHandler> menu_click_handler(
      e_menu_item item ) const;
};

void draw_all_planes( Texture& tx = Texture::screen() );

// This will call the on_frame_{start,end} method on each plane
// to update any state that it has. It will only be called on
// frames that are enabled and visible.
void update_all_planes_start();
void update_all_planes_end();

// Returns true if one of the planes handled the input, false
// otherwise. At most one plane will handle the input.
bool send_input_to_planes( input::event_t const& event );

} // namespace rn
