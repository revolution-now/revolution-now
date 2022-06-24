/****************************************************************
**terrain.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-23.
*
# Description: Describes possible terrain types.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// Rds
#include "gs/terrain-enums.rds.hpp"

namespace rn {

/****************************************************************
** e_terrain
*****************************************************************/
bool is_land( e_terrain terrain );
bool is_water( e_terrain terrain );

e_surface surface_type( e_terrain terrain );

// This includes either clearing a forest or irrigating.
bool can_plow( e_terrain terrain );

// What will the terrain type become when cleared (if possible).
maybe<e_ground_terrain> cleared_forest( e_terrain terrain );

bool has_forest( e_terrain terrain );

// This does not include clearing forests.
bool can_irrigate( e_terrain terrain );

// If the effective terrain is also a ground terrain then this
// will return it.
maybe<e_ground_terrain> to_ground_terrain( e_terrain terrain );

// Note: in general calling this function is not sufficient to
// get the effective terrain from a map square, since it doesn't
// take into account whether e.g. there is a forest on the
// square. This is only for ground terrain.
e_terrain from_ground_terrain( e_ground_terrain ground );

} // namespace rn
