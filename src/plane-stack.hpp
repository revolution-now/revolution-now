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
 private:
  // This both holds a group of planes and also serves as an raii
  // cleanup object.
  struct [[nodiscard]] PlaneGroupOwner {
    PlaneGroupOwner( Planes& planes ) : planes_( planes ) {
      prev_ = planes_.group_;
      if( prev_ ) {
        group.omni    = prev_->omni;
        group.console = prev_->console;
        group.window  = prev_->window;
      }
      planes_.group_ = &group;
    }
    ~PlaneGroupOwner() noexcept { planes_.group_ = prev_; }
    PlaneGroupOwner( PlaneGroupOwner&& ) = delete;

    Planes&     planes_;
    PlaneGroup* prev_ = nullptr;
    PlaneGroup  group;
  };

 public:
  Planes();
  Planes( Planes&& ) = delete;

  PlaneGroupOwner push();

  PlaneGroup const& get() const;
  PlaneGroup&       get();

 private:
  PlaneGroup* group_ = nullptr;
  // This must be declared after group_ because its constructor
  // will refer to it.
  PlaneGroupOwner initial_;
};

} // namespace rn
