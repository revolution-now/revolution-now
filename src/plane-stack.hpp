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

// base
#include "base/macros.hpp"

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
** PlaneGroup
*****************************************************************/
struct PlaneGroup {
  PlaneGroup() = default;

  PlaneGroup( std::vector<Plane*> planes );

  // This will push a plane onto the top, so that it will be
  // drawn on top.
  template<typename T>
  void push( T& plane_wrapper ) {
    push_impl( &plane_wrapper.impl() );
  }

  std::vector<Plane*> const& all() const { return planes_; }

 private:
  void push_impl( Plane* plane );

  std::vector<Plane*> planes_;
};

/****************************************************************
** Planes
*****************************************************************/
struct Planes {
  // Get's the single global planes object. Should only be used
  // by functions which cannot accept parameters. This is pretty
  // much ok because these plane stack objects don't hold any
  // real state themselves; they just hold pointers to real Plane
  // objects that will exist on the call stack and will add and
  // remove themselves using RAII.
  static Planes& global();

  Planes() = default;

 private:
  struct [[nodiscard]] popper {
    popper( Planes& planes ) : planes_( planes ) {}
    NO_COPY_NO_MOVE( popper );
    ~popper();
    Planes& planes_;
  };

 public:
  // To add a new plane group, first call this followed by back()
  // to get the PlaneGroup, then call push on the plane group.
  popper new_group();

  PlaneGroup& back() { return groups_.back(); }

  void draw( rr::Renderer& renderer ) const;

  // This will call the advance_state method on each plane to up-
  // date any state that it has. It will only be called on frames
  // that are enabled and visible.
  void advance_state();

  e_input_handled send_input( input::event_t const& event );

 private:
  std::vector<PlaneGroup> groups_;

  enum class e_drag_send_mode { normal, raw, motion };

  struct DragState {
    Plane*           plane;
    e_drag_send_mode mode = e_drag_send_mode::normal;
  };

  maybe<DragState> drag_state_;
};

} // namespace rn
