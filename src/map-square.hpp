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

// Revolution Now
#include "mv-points.hpp"

// Rds
#include "map-square.rds.hpp"

namespace rn {

bool is_land( MapSquare const& square );
bool is_water( MapSquare const& square );

e_surface surface_type( MapSquare const& square );

MovementPoints movement_points_required(
    MapSquare const& src_square, MapSquare const& dst_square );

} // namespace rn
