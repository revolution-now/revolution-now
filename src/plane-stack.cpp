/****************************************************************
**plane-stack.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-17.
*
* Description: API for adding and removing plane groups where
*              plane groups are held on the stack and added and
*              removed via raii.
*
*****************************************************************/
#include "plane-stack.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Planes
*****************************************************************/
Planes::Planes() : group_( nullptr ), initial_( *this ) {}

Planes::PlaneGroupOwner Planes::push() {
  return PlaneGroupOwner( *this );
}

PlaneGroup const& Planes::get() const {
  CHECK( group_ != nullptr );
  return *group_;
}

PlaneGroup& Planes::get() {
  CHECK( group_ != nullptr );
  return *group_;
}

Planes::PlaneGroupOwner::PlaneGroupOwner( Planes& planes )
  : planes_( planes ) {
  prev_ = planes_.group_;
  if( prev_ ) {
    group.omni    = prev_->omni;
    group.console = prev_->console;
    group.window  = prev_->window;
    group.menu    = prev_->menu;
  }
  planes_.group_ = &group;
}

Planes::PlaneGroupOwner::~PlaneGroupOwner() noexcept {
  planes_.group_ = prev_;
}

} // namespace rn
