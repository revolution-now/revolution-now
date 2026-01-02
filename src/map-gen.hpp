/****************************************************************
**map-gen.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Game map generator.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// gfx
#include "gfx/coord.hpp"

namespace lua {
struct state;
}

namespace rn {

struct IEngine;
struct IMapUpdater;
struct SS;
struct TerrainConnectivity;

void ascii_map_gen( IEngine& engine );

void reset_terrain( IMapUpdater& map_updater, Delta size );

} // namespace rn
