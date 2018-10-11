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

#include "base-util.hpp"
#include "macros.hpp"

namespace rn {

namespace {} // namespace

MovementPoints::MovementPoints( int integral, int atoms ) {
  points_atoms = ( ( integral + ( atoms / factor ) ) * factor ) +
                 ( atoms % factor );
}

} // namespace rn
