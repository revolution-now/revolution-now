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

// rds
#include "biomes.rds.hpp"

// config
#include "config/map-gen-types.rds.hpp"

// base
#include "base/valid.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct BiomeWetDryModulation;
struct IRand;
struct MapMatrix;
struct RealTerrain;

/****************************************************************
** Public API.
*****************************************************************/
base::valid_or<std::string> assign_biomes(
    IRand& rand, RealTerrain& real_terrain,
    WeatherValue temperature, WeatherValue climate );

void apply_biome_wetness_remix(
    MapMatrix& m, IRand& rand,
    BiomeWetDryModulation const& config );

void assign_arctic_biomes( IRand& rand,
                           RealTerrain& real_terrain );

} // namespace rn
