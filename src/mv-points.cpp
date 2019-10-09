/****************************************************************
**mv-points.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-02.
*
* Description: A type for representing movement points that will
*              ensure correct handling of atomsal points.
*
*****************************************************************/
#include "mv-points.hpp"

// {fmt}
#include "fmt/format.h"

namespace rn {

namespace {} // namespace

MovementPoints::MovementPoints( int integral, int atoms ) {
  points_atoms = ( ( integral + ( atoms / factor ) ) * factor ) +
                 ( atoms % factor );
}

std::string MovementPoints::to_string() const {
  if( points_atoms % factor == 0 )
    return fmt::format( "{:d}", points_atoms / factor );
  else
    return fmt::format( "{:d} {:d}/{:d}", points_atoms / factor,
                        points_atoms % factor, factor );
}

expect<> MovementPoints::check_invariants_safe() const {
  if( points_atoms < 0 )
    return UNEXPECTED(
        "MovementPoints object has negative points" );
  return xp_success_t{};
}

} // namespace rn
