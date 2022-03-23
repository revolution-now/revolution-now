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
#include "terrain.hpp"

// Rds
#include "terrain.rds.hpp"

namespace rn {

bool is_land( e_terrain terrain );
bool is_water( e_terrain terrain );

e_surface surface_type( e_terrain terrain );

} // namespace rn
