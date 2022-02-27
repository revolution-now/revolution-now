/****************************************************************
**cartesian.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-26.
*
* Description: Structs for working in cartesian space.
*
*****************************************************************/
#include "cartesian.hpp"

using namespace std;

namespace gfx {

/****************************************************************
** size
*****************************************************************/
size size::max_with( size const& rhs ) const {
  return size{ std::max( w, rhs.w ), std::max( h, rhs.h ) };
}

/****************************************************************
** point
*****************************************************************/
point const& point::origin() {
  static point p = {};
  return p;
}

/****************************************************************
** Combining Operators
*****************************************************************/
point operator+( point const& p, size const& s ) {
  return point{ .x = p.x + s.w, .y = p.y + s.h };
}

point operator+( size const& s, point const& p ) {
  return p + s;
}
} // namespace gfx
