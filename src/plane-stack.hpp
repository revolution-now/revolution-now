/****************************************************************
**plane-stack.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-08.
*
* Description: API for adding and removing plane groups where
*              plane groups are held on the stack and added and
*              removed via raii.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "plane-group.hpp"

namespace rn {

/****************************************************************
** Planes
*****************************************************************/
struct Planes {
  // This both holds a group of planes and also serves as an raii
  // cleanup object.
  struct [[nodiscard]] PlaneGroupOwner {
    PlaneGroupOwner( Planes& planes );
    ~PlaneGroupOwner() noexcept;
    PlaneGroupOwner( PlaneGroupOwner&& ) = delete;

    Planes& planes_;
    PlaneGroupOwner* prev_owner_ = nullptr;
    PlaneGroup group;
  };

  Planes();
  Planes( Planes&& ) = delete;

  PlaneGroupOwner push();

  PlaneGroup const& get() const;
  PlaneGroup& get();

  void on_logical_resolution_changed( e_resolution resolution );

 private:
  template<typename Func>
  void on_all_groups( Func const& fn ) const;

  // Effectively a linked list of PlaneGroupOwner objects, each
  // of which will be located on the stack in some stack frame.
  // This will initially be set to point to the initial_ plane
  // group owner below during its constructor.
  PlaneGroupOwner* owner_head_ = nullptr;
  // This must be declared after group_ because its constructor
  // will refer to it.
  PlaneGroupOwner initial_;
};

} // namespace rn
