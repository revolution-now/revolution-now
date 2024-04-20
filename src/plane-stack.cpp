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

} // namespace rn
