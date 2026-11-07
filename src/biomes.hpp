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

// base
#include "base/valid.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IRand;
struct MapMatrix;
struct RealTerrain;
struct WeatherValue;
struct WetnessConfig;
struct Wetness;

/****************************************************************
** Public API.
*****************************************************************/
void assign_arctic_biomes( IRand& rand,
                           RealTerrain& real_terrain );

base::valid_or<std::string> assign_biomes(
    IRand& rand, RealTerrain& real_terrain,
    WeatherValue const& temperature,
    WeatherValue const& climate );

void apply_biome_wetness_remix(
    MapMatrix& m, IRand& rand,
    WetnessConfig const& wetness_config,
    Wetness const& wetness );

} // namespace rn
