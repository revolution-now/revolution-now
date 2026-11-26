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
struct GeneratedMapSetup;
struct IRand;
struct MapMatrix;
struct RiverParameters;
struct WeatherValue;

/****************************************************************
** TerrainGenerator.
*****************************************************************/
struct TerrainGenerator {
  TerrainGenerator( MapMatrix& m, IRand& rand,
                    GeneratedMapSetup const& setup );

  using Result = base::valid_or<std::string>;

  Result full();

  Result elevate();

  Result temperature_pass();

  // Left-to-right.
  Result rainfall_pass_1();

  // Right-to-left.
  Result rainfall_pass_2();

  Result scatter_pass();

  Result forest_pass();

  Result rivers_pass( RiverParameters const& params );

  Result arctic_tundra_pass();

 private:
  MapMatrix& m_;
  IRand& rand_;
  GeneratedMapSetup const& setup_;
  gfx::size const sz_;
  double const density_ = {};
  double const scale_;
  WeatherValue const& temperature_;
  WeatherValue const& climate_;

  [[nodiscard]] bool ocean(
      gfx::point const p ) const; // FIXME: remove
  void enable_hills( gfx::point const p );
  void enable_mountains( gfx::point const p );
  [[nodiscard]] bool ocean_adjacent( gfx::point const p ) const;

  template<typename F>
  void on_all( F&& fn ) {
    on_all_tiles( m_, fn );
  }

  [[nodiscard]] double scale_h() const;
  [[nodiscard]] double scale_w() const;
  [[nodiscard]] double scale_area() const;
  [[nodiscard]] double scale_linear() const;

  [[nodiscard]] int uniform( int const l, int const h ) const;
  [[nodiscard]] bool coin() const;

  [[nodiscard]] int uniform_from_1_to( int d ) const;
  [[nodiscard]] int uniform_from_1_to( double d ) const;
  [[nodiscard]] int uniform_from_0_to( int d ) const;
  [[nodiscard]] int uniform_from_0_to( double d ) const;

  [[nodiscard]] double og_land_mass() const;
  [[nodiscard]] double og_land_form() const;
  [[nodiscard]] double og_temperature() const;
  [[nodiscard]] double og_climate() const;
  [[nodiscard]] int og_land_mass_int() const;
  [[nodiscard]] int og_land_form_int() const;
  [[nodiscard]] int og_temperature_int() const;
  [[nodiscard]] int og_climate_int() const;

  base::maybe<gfx::point> random_tile(
      int max_tries,
      base::function_ref<bool( gfx::point, MapSquare const& )
                             const>
          pred ) const;

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
