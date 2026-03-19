/****************************************************************
**biomes.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-02-13.
*
* Description: Assigns ground terrain types to a map.
*
*****************************************************************/
#pragma once

// config
#include "config/map-gen-types.rds.hpp"

// base
#include "base/valid.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IRand;
struct RealTerrain;

/****************************************************************
** Public API.
*****************************************************************/
base::valid_or<std::string> assign_biomes(
    IRand& rand, RealTerrain& real_terrain,
    WeatherValue temperature, WeatherValue climate );

} // namespace rn
