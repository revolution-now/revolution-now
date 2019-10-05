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

SERIALIZABLE_STRUCT_DEF( MovementPoints ) {
  return fb::MovementPoints( points_atoms );
}

} // namespace rn
