/****************************************************************
**iplane-group.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-17.
*
* Description: Ordered list of planes that implements the IPlane
*              interface.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "maybe.hpp"
#include "plane.hpp"

// C++ standard library
#include <vector>

namespace gfx {
enum class e_resolution;
}

namespace rn {

/****************************************************************
** IPlaneGroup
*****************************************************************/
struct IPlaneGroup {
  virtual ~IPlaneGroup() = default;

  virtual std::vector<IPlane*> planes() const = 0;

 public:
  void draw( rr::Renderer& renderer ) const;

  void advance_state();

  e_input_handled input( input::event_t const& event );

  void on_logical_resolution_changed(
      gfx::e_resolution resolution );

 private:
  // Drag state.
  enum class e_drag_send_mode {
    normal,
    raw,
    motion
  };
  struct DragState {
    IPlane* plane;
    e_drag_send_mode mode = e_drag_send_mode::normal;
  };
  maybe<DragState> drag_state_;
};

} // namespace rn
