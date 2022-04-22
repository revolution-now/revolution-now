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

// Revolution Now
#include "game-state.hpp"
#include "map-updater.hpp"

namespace rn {

void generate_terrain( IMapUpdater& map_updater );

void linker_dont_discard_module_map_gen();

} // namespace rn
