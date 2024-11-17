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

namespace rn {

/****************************************************************
** IPlaneGroup
*****************************************************************/
struct IPlaneGroup : public IPlane {
  virtual std::vector<IPlane*> planes() const = 0;

 public: // IPlane
  void draw( rr::Renderer& renderer ) const override final;

  void advance_state() override final;

  e_input_handled input(
      input::event_t const& event ) override final;

  void on_logical_resolution_changed(
      e_resolution resolution ) override final;

 public: // IPlaneGroup
  // This is used to ensure that the plane group gets initialized
  // once after it is create and before it is used.
  void initialize_if_needed(
      maybe<e_resolution> const resolution );

 private:
  // Drag state.
  enum class e_drag_send_mode {
    normal,
    raw,
    motion
  };
  struct DragState {
    IPlane*          plane;
    e_drag_send_mode mode = e_drag_send_mode::normal;
  };
  maybe<DragState> drag_state_;

  // Initialization.
  bool initialized_ = false;
};

} // namespace rn
