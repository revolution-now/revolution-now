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

namespace {

using ::gfx::e_resolution;

}

/****************************************************************
** Planes
*****************************************************************/
Planes::Planes() : initial_( *this ) {}

Planes::PlaneGroupOwner Planes::push() {
  return PlaneGroupOwner( *this );
}

PlaneGroup const& Planes::get() const {
  CHECK( owner_head_ != nullptr );
  return owner_head_->group;
}

PlaneGroup& Planes::get() {
  CHECK( owner_head_ != nullptr );
  return owner_head_->group;
}

Planes::PlaneGroupOwner::PlaneGroupOwner( Planes& planes )
  : planes_( planes ) {
  prev_owner_ = planes_.owner_head_;
  if( prev_owner_ ) {
    group.omni    = prev_owner_->group.omni;
    group.console = prev_owner_->group.console;
    group.window  = prev_owner_->group.window;
    group.menu    = prev_owner_->group.menu;
  }
  planes_.owner_head_ = this;
}

Planes::PlaneGroupOwner::~PlaneGroupOwner() noexcept {
  planes_.owner_head_ = prev_owner_;
}

template<typename Func>
void Planes::on_all_groups( Func const& fn ) const {
  PlaneGroupOwner* curr = owner_head_;
  CHECK( curr != nullptr );

  while( true ) {
    fn( curr->group );
    if( curr == &initial_ ) break;
    curr = curr->prev_owner_;
    CHECK( curr != nullptr );
  }
}

// When the logical resolution changed we not only need to send a
// signal to the active planes, but also to the inactive ones in
// burried plane groups so that when they become active again
// they will have the correct idea of the resolution.
void Planes::on_logical_resolution_changed(
    e_resolution const resolution ) {
  on_all_groups( [&]( PlaneGroup& group ) {
    group.on_logical_resolution_changed( resolution );
  } );
}

} // namespace rn
