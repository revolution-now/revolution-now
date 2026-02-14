/****************************************************************
**biomes.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-02-13.
*
* Description: Assigns ground terrain types to a map.
*
*****************************************************************/
#include "biomes.hpp"

// Revolution Now
#include "error.hpp"
#include "irand.hpp"

// ss
#include "ss/terrain.hpp"

// config
#include "config/map-gen.rds.hpp"
#include "terrain-enums.rds.hpp"

// base
#include "base/expect.hpp"
#include "base/logger.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::expect;
using ::base::maybe;
using ::base::valid;
using ::base::valid_or;
using ::gfx::point;
using ::gfx::size;
using ::refl::enum_map;
using ::refl::enum_values;

/****************************************************************
** biome_curve
*****************************************************************/
struct biome_curve {
  [[maybe_unused]] biome_curve() = default;

  static expect<biome_curve> create( e_ground_terrain const gt,
                                     int const temperature,
                                     int const climate ) {
    biome_curve res;
    auto const& conf = config_map_gen.terrain_generation.biomes;
    res.curve_       = conf.normal_temperature_and_climate[gt];
    double const d_temperature =
        -clamp( temperature, -100, 100 ) / 100.0;
    double const d_climate =
        -clamp( climate, -100, 100 ) / 100.0;

    auto const apply_gradient = [&]( auto member ) {
      res.curve_.params.*member *=
          ( 1 + conf.temperature_gradient[gt].*member *
                    d_temperature );
      res.curve_.params.*member *=
          ( 1 + conf.climate_gradient[gt].*member * d_climate );
    };

    apply_gradient( &config::map_gen::BiomeCurveParams::weight );
    apply_gradient( &config::map_gen::BiomeCurveParams::center );
    apply_gradient( &config::map_gen::BiomeCurveParams::stddev );
    apply_gradient( &config::map_gen::BiomeCurveParams::sub );

    GOOD_OR_RETURN( res.curve_.validate() );
    return res;
  }

  [[nodiscard]] double operator()( double const d ) const {
    CHECK_GE( d, 0.0 );
    CHECK_LE( d, 1.0 );
    // The sqrt( 2 ) here doesn't have the same meaning for
    // curves with exponents other than 2, but we'll leave it in
    // so that the formulas are the same except for exponent.
    static double const sqrt_2 = sqrt( 2.0 );

    auto const sample = [&]( double const x ) {
      double const a      = curve_.params.weight;
      double const mean   = curve_.params.center / 2.0 + 0.5;
      double const stddev = curve_.params.stddev / 2.0;
      double const sub    = curve_.params.sub;
      double const xp     = curve_.exp;
      return a *
             ( exp( -pow( ( ( x - mean ) / ( stddev * sqrt_2 ) ),
                          xp ) ) -
               sub );
    };
    // The gaussians come in pairs in a way that is symmetric
    // about the equator, so when we sample one map row we need
    // to add the contributions from both.
    return sample( d ) + sample( 1.0 - d );
  }

 private:
  config::map_gen::BiomeCurve curve_;
};

struct biome_dist {
  biome_dist(
      enum_map<e_ground_terrain, biome_curve> const& curves,
      double const o ) {
    double total = 0;
    for( auto const gt : enum_values<e_ground_terrain> ) {
      // Some curves can go below zero, e.g. the savannah and
      // swamp curves which are lowered slightly below the axis.
      // In that case they are interpreted as being zero in that
      // region.
      double const val = std::max( curves[gt]( o ), 0.0 );
      weights_[gt]     = val;
      total += val;
    }
    // The function we use below to select a random weighted
    // value does not need the values to be normalized, however
    // it does fail if the total weight is considered too small,
    // so we need to normalize it just to prevent that from hap-
    // pening.
    //
    // FIXME: find a better way to deal with this, e.g. don't
    //        allow creating the biome_dist object.
    //
    if( total > 0.0 )
      for( auto const gt : enum_values<e_ground_terrain> )
        weights_[gt] /= total;
    else
      for( auto const gt : enum_values<e_ground_terrain> )
        weights_[gt] = 0.0;
  }

  [[nodiscard]] maybe<e_ground_terrain> sample(
      IRand& rand ) const {
    return rand.pick_from_weighted_values_safe( weights_ );
  }

  string to_str( e_ground_terrain const gt ) const {
    return format( "dist.{}={}", gt, weights_[gt] );
  }

 private:
  enum_map<e_ground_terrain, double> weights_;
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
valid_or<string> assign_biomes( IRand& rand,
                                RealTerrain& real_terrain,
                                int const temperature,
                                int const climate ) {
  auto& m       = real_terrain.map;
  size const sz = m.size();

  enum_map<e_ground_terrain, biome_curve> curves;
  for( auto const gt : enum_values<e_ground_terrain> ) {
    UNWRAP_RETURN(
        curve, biome_curve::create( gt, temperature, climate ) );
    curves[gt] = curve;
  }

  // This should have been checked via the config validators
  // and/or map key validators.
  CHECK( sz.h % 2 == 0 );
  CHECK( sz.h > 1 );

  for( int y = 0; y < sz.h; ++y ) {
    double const yd = double( y ) / sz.h;
    biome_dist const dist( curves, yd );
    for( int x = 0; x < m.size().w; ++x ) {
      MapSquare& square = m[{ .x = x, .y = y }];
      // There would be no harm in distributing ground types on
      // water tiles, but we won't do it 1) for performance rea-
      // sons, and 2) because the rendering engine doesn't look
      // at them anyway (for water tiles that have some land on
      // the edges it will take the ground type from an adjacent
      // tile which is better for visual continuity anyway).
      if( square.surface == e_surface::water ) continue;
      auto const sample = dist.sample( rand );
      if( !sample.has_value() ) {
        for( auto const gt : enum_values<e_ground_terrain> )
          lg.warn( "{}: {}\n", gt, dist.to_str( gt ) );
        return format( "failed to sample biome at map row {}",
                       y );
      }
      square.ground = *sample;
    }
  }

  return valid;
}

} // namespace rn
