/****************************************************************
**land-production.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: Computes what is produced on a land square.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "colony.rds.hpp"

namespace rn {

struct TerrainState;

int production_on_square( e_outdoor_job       job,
                          TerrainState const& terrain_state,
                          Coord               where );

} // namespace rn
