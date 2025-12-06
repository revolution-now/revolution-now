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

// Revolution Now
#include "error.hpp"
#include "ui.hpp"

// gfx
#include "gfx/coord.hpp"

/****************************************************************
** Forward Decls.
*****************************************************************/
namespace gfx {
enum class e_resolution;
}

namespace rn::input {
struct mod_keys;
enum class e_mouse_button;
} // namespace rn::input

namespace rn {

/****************************************************************
** IPlane
*****************************************************************/
struct IPlane : public ui::object {
  IPlane() = default;

 public: // ui::object
  Delta delta() const override { NOT_IMPLEMENTED; }

  void draw( rr::Renderer& renderer,
             Coord coord ) const override;

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
  [[nodiscard]] e_accept_drag virtual can_drag(
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

  // ------------------------------------------------------------
  // Resolutions
  // ------------------------------------------------------------
  // Called whenever the logical resolution changes and hence
  // when things may need to have their layout changed.
  virtual void on_logical_resolution_changed(
      gfx::e_resolution resolution ) final;

  // Asks the plane if it has a dedicated layout for the given
  // resolution.
  [[nodiscard]] virtual bool supports_resolution(
      gfx::e_resolution resolution ) const;

 protected:
  // This will be called when a rendering resolution has been se-
  // lected for the plane to use. Note that the selected resolu-
  // tion can be different from the actual logical resolution in
  // cases where the plane does not have a layout for the actual
  // resolution but has one for another resolution that can be
  // subsumed by the actual resolution. This is used to help
  // planes render on logical resolutions that they don't yet
  // have a layout for, in order to make it easier to add new
  // resolutions and implement them gradually for all planes.
  virtual void on_logical_resolution_selected(
      gfx::e_resolution resolution );
};

struct NoOpPlane : IPlane {};

static_assert( !std::is_abstract_v<NoOpPlane> );

} // namespace rn
