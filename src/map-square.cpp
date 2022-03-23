/****************************************************************
**map-square.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-23.
*
* Description: Represents a single square of land.
*
*****************************************************************/
#include "map-square.hpp"

// Revolution Now
#include "terrain.hpp"

using namespace std;

namespace rn {

bool is_land( MapSquare const& square ) {
  return is_land( square.terrain );
}

bool is_water( MapSquare const& square ) {
  return is_water( square.terrain );
}

e_surface surface_type( MapSquare const& square ) {
  return surface_type( square.terrain );
}

} // namespace rn
