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

// Revolution Now
#include "map-stats.hpp"

// config
#include "config/map-gen-types.rds.hpp"

// ss
#include "ss/terrain-enums.rds.hpp"

// refl
#include "refl/enum-map.hpp"

// base
#include "base/valid.hpp"

// C++ standard library
#include <map>

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IRand;
struct RealTerrain;

/****************************************************************
** BiomeDensityStatsCollector
*****************************************************************/
// TODO: create a factory function for this that returns
// unique_pt<IMapStatsCollector> and move this into the cpp.
struct BiomeDensityStatsCollector : IMapStatsCollector {
  BiomeDensityStatsCollector( std::string const& stem );

 public: // IMapStatsCollector
  void collect( MapMatrix const& m ) override;
  void summarize() override;
  void write() const override;

 private:
  std::string const stem_;
  gfx::size map_sz_ = {};
  std::map<int /*y+1*/,
           std::map<e_ground_terrain, int /*count*/>>
      biome_count_y_;
  std::map<int /*y+1*/, int> land_count_y_;
  int land_total_ = 0;
  int maps_total_ = 0;
};

/****************************************************************
** BiomeAdjacencyStatsCollector
*****************************************************************/
template<typename T>
struct BiomeWetStats {
  T with_swamp_adjacent          = {};
  T with_marsh_adjacent          = {};
  T with_swamp_or_marsh_adjacent = {};
  T with_river_adjacent          = {};
  T with_ocean_adjacent          = {};
  T with_river_or_ocean_adjacent = {};
  T with_any_adjacent            = {};
  T with_river                   = {};

  void on_all( base::function_ref<void( T )> fn ) const {
    fn( with_swamp_adjacent );
    fn( with_marsh_adjacent );
    // fn( with_swamp_or_marsh_adjacent );
    fn( with_river_adjacent );
    fn( with_ocean_adjacent );
    // fn( with_river_or_ocean_adjacent );
    // fn( with_any_adjacent );
    // fn( with_river );
  }
};

// TODO: create a factory function for this that returns
// unique_pt<IMapStatsCollector> and move this into the cpp.
struct BiomeAdjacencyStatsCollector : IMapStatsCollector {
  BiomeAdjacencyStatsCollector(
      BiomeClustering const& clustering );

 public: // IMapStatsCollector
  void collect( MapMatrix const& m ) override;
  void summarize() override;
  void write() const override;

 private:
  gfx::size map_sz_ = {};
  refl::enum_map<e_ground_terrain, BiomeWetStats<double>> wet_;
  refl::enum_map<e_ground_terrain, int> adjacency_;
  refl::enum_map<e_ground_terrain, int> terrain_total_;
  refl::enum_map<e_ground_terrain,
                 std::map<int /*y*/, int /*count*/>>
      ground_per_row_;
  std::map<int /*y*/, int /*count*/> land_per_row_;
  // This gives the total number of surrounding land squares (of
  // any biome) around tiles of the given biome on the given row.
  refl::enum_map<e_ground_terrain,
                 std::map<int /*y*/, int /*count*/>>
      surrounding_per_row_;
  int land_total_ = 0;
  int maps_total_ = 0;

 public: // Summary.
  BiomeClustering const clustering_;
  refl::enum_map<e_ground_terrain, double> adjacency_baselines;
  refl::enum_map<e_ground_terrain, double> relative_adjacencies;
  refl::enum_map<e_ground_terrain, BiomeWetStats<double>>
      wet_results;
  refl::enum_map<e_ground_terrain, BiomeWetStats<double>>
      wet_ratios;
};

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
    BiomeClustering const& clustering );

} // namespace rn
