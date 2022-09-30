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

struct IMapUpdater;
struct SS;

void generate_terrain( lua::state&  st,
                       IMapUpdater& map_updater );

void ascii_map_gen( lua::state& st, SS& ss );

void reset_terrain( IMapUpdater& map_updater, Delta size );

} // namespace rn
