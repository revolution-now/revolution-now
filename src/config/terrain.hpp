/****************************************************************
**terrain.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-24.
*
# Description: Config info for terrain.
*
*****************************************************************/
#pragma once

// Rds
#include "config/terrain.rds.hpp"

// base
#include "base/maybe.hpp"

namespace rn {

// If the effective terrain is also a ground terrain then this
// will return it.
base::maybe<e_ground_terrain> to_ground_terrain(
    e_terrain terrain );

// Note: in general calling this function is not sufficient to
// get the effective terrain from a map square, since it doesn't
// take into account whether e.g. there is a forest on the
// square. This is only for ground terrain.
e_terrain from_ground_terrain( e_ground_terrain ground );

} // namespace rn
