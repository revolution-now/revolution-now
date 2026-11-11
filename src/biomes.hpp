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
#include "terrain-mgr.hpp"

// gfx
#include "gfx/matrix.hpp"

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

base::valid_or<std::string> assign_biomes_old(
    IRand& rand, RealTerrain& real_terrain,
    WeatherValue const& temperature,
    WeatherValue const& climate );

void apply_biome_wetness_remix(
    MapMatrix& m, IRand& rand,
    WetnessConfig const& wetness_config,
    Wetness const& wetness );

/****************************************************************
** TerrainGenerator.
*****************************************************************/
struct TerrainGenerator {
  TerrainGenerator( MapMatrix& m, IRand& rand,
                    WeatherValue const& temperature,
                    WeatherValue const& climate );

  using Result = base::valid_or<std::string>;

  Result full();

  Result temperature_pass();

  // Left-to-right.
  Result wetness_pass_1();

  // Right-to-left.
  Result wetness_pass_2();

  Result scatter_pass();

  Result forest_pass();

 private:
  MapMatrix& m_;
  IRand& rand_;
  gfx::size const sz_;
  WeatherValue const& temperature_;
  WeatherValue const& climate_;

  template<typename F>
  void on_all( F&& fn ) {
    on_all_tiles( m_, fn );
  }

  enum class e_forest_plan {
    deferred,
    suppress,
    populate,
  };
  struct Scratch {
    e_forest_plan forest_plan = e_forest_plan::deferred;
  };
  gfx::matrix<Scratch> scratch_;
};

} // namespace rn
