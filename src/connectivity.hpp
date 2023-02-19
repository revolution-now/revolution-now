/****************************************************************
**connectivity.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-13.
*
* Description: Computes connectivity of bodies of land and sea.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// rds
#include "connectivity.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct SSConst;

// Scans the entire map and computes the set of connected seg-
// ments of land and water, where "connected" means as a unit
// would move (i.e., it includes diagonals). This is not cheap to
// compute, so should only be computed when necessary.
void update_terrain_connectivity( SSConst const&       ss,
                                  TerrainConnectivity* out );

// Should be called with the location of a water tile and this
// will determine if it has access to one of the edges of the map
// (either left or right). The assumption here is that if the
// tile is connected to the left or right edge of the map then it
// is deemed to have "ocean" access, since it will have access to
// at least one sea lane.
bool has_ocean_access( TerrainConnectivity const& conn,
                       Coord                      tile );

// Ditto but only for the left edge of the map (left sea lane).
bool has_left_ocean_access( TerrainConnectivity const& conn,
                            Coord                      tile );

// Ditto but only for the right edge of the map (left sea lane).
bool has_right_ocean_access( TerrainConnectivity const& conn,
                             Coord                      tile );

} // namespace rn
