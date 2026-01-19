/****************************************************************
**map-gen.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Map-generator related helpers.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IMapUpdater;
struct IRand;
struct RealTerrain;
struct TerrainState;

/****************************************************************
** Public API.
*****************************************************************/
void remove_islands( RealTerrain& real_terrain );

void remove_crosses( RealTerrain& real_terrain );

[[nodiscard]] gfx::rect compute_land_zone( gfx::size world_sz );

void reset_terrain( IMapUpdater& map_updater, gfx::size sz );

// Returns realized density.
double place_arctic( RealTerrain& real_terrain, IRand& rand,
                     double density );

void generate_proto_tiles( TerrainState& root );

} // namespace rn
