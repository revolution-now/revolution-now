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
struct IRand;
struct MapMatrix;
struct RealTerrain;

/****************************************************************
** Public API.
*****************************************************************/
BiomeClustering biome_clustering_for_climate(
    e_climate climate );

BiomeClustering biome_clustering_for_climate(
    WeatherValue climate );

base::valid_or<std::string> assign_biomes(
    IRand& rand, RealTerrain& real_terrain,
    WeatherValue temperature, WeatherValue climate,
    double wet_dry_sensitivity );

base::expect<WetnessAdjustmentResult> adjust_biome_wetness(
    IRand& rand, MapMatrix& m, double wet_dry_sensitivity );

base::expect<AdjacencyAdjustmentResult> adjust_biome_clustering(
    IRand& rand, RealTerrain& real_terrain,
    WeatherValue const temperature, WeatherValue const climate,
    BiomeClustering const& clustering );

void assign_arctic_biomes( IRand& rand,
                           RealTerrain& real_terrain );

void log_adjacency_results(
    AdjacencyAdjustmentResult const& result );

} // namespace rn
