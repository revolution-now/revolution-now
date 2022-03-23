/****************************************************************
**map-square.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-02-01.
*
* Description: Represents a single square of land.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "map-square.rds.hpp"

namespace rn {

bool is_land( MapSquare const& square );
bool is_water( MapSquare const& square );

e_surface surface_type( MapSquare const& square );

} // namespace rn
