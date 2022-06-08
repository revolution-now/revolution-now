/****************************************************************
**plane-stack.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-08.
*
* Description: A (quasi) stack for storing the list of active
*              planes.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "input.hpp" // FIXME

// Rds
#include "plane-stack.rds.hpp"

// C++ standard library
#include <vector>

namespace rr {
struct Renderer;
}

namespace rn {

/****************************************************************
** Forward Decls
*****************************************************************/
struct input_event;
struct IMapUpdater;
struct Plane;

/****************************************************************
** Planes
*****************************************************************/
struct Planes {
  // Push a plane onto the end, that means that it will be drawn
  // on top of the ones before it.
  void push( Plane& plane );

  void pop();

  void draw_all_planes( rr::Renderer& renderer );

  // This will call the advance_state method on each plane to up-
  // date any state that it has. It will only be called on frames
  // that are enabled and visible.
  void advance_state();

  [[nodiscard]] e_input_handled send_input(
      input::event_t const& event );

  std::vector<Plane*> const& all() const { return planes_; }

 private:
  std::vector<Plane*> planes_;
};

/****************************************************************
** PlaneStack
*****************************************************************/
struct PlaneStack {
  // Get's the single global plane groups state. Should only be
  // used by functions which cannot accept parameters. This is
  // pretty much ok because these plane stack objects don't hold
  // any real state themselves; they just hold pointers to real
  // Plane objects that will exist on the call stack and will add
  // and remove themselves using RAII.
  static PlaneStack& global();

  PlaneStack();

  Planes& operator[]( e_plane_stack_level level );

  void draw_all_planes( rr::Renderer& renderer );

  void advance_state();

  e_input_handled send_input( input::event_t const& event );

  std::vector<Planes>&       all() { return groups_; }
  std::vector<Planes> const& all() const { return groups_; }

 private:
  std::vector<Planes> groups_;

  enum class e_drag_send_mode { normal, raw, motion };

  struct DragState {
    Plane*           plane;
    e_drag_send_mode mode = e_drag_send_mode::normal;
  };

  maybe<DragState> drag_state_;
};

} // namespace rn
