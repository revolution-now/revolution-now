/****************************************************************
**perlin-map.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-10.
*
* Description: Perlin Noise based map generation routines.
*
*****************************************************************/
#include "perlin-map.hpp"

// gfx
#include "gfx/iter.hpp"

// rand
#include "perlin-map.rds.hpp"
#include "rand/perlin.hpp"

// rand
#include "rand/entropy.hpp"

// refl
#include "refl/traverse.hpp"
#include "refl/validate.hpp"

// traverse
#include "traverse/ext-base.hpp"
#include "traverse/ext-std.hpp"
#include "traverse/ext.hpp"

// base
#include "base/error.hpp"
#include "base/logger.hpp"
#include "base/timer.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <algorithm>

using namespace std;

namespace rn {

namespace {

using ::base::expect;
using ::base::ScopedTimer;
using ::base::valid;
using ::base::valid_or;
using ::gfx::dsize;
using ::gfx::matrix;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::rect_iterator;
using ::gfx::size;
using ::std::max;
using ::std::min;

enum class e_sea_level {
  too_low,
  good,
  too_high,
};

// This edge-suppression algo only makes sense with the Perlin
// generator because it works by lowering the "height" of the
// tiles near the edges, making it more likely that they will end
// up under the "sea level".
void perlin_suppress_edges(
    point const p, size const map_sz,
    PerlinEdgeSuppression const& edge_suppression,
    double& level ) {
  dsize const dist_unscaled = ( p - map_sz / 2 )
                                  .distance_from_origin()
                                  .abs()
                                  .to_double();
  // This will have the effect of doing less suppression at the
  // edges as the map size grows, which looks better. Don't let
  // it go above 1 otherwise small maps will have too much re-
  // moved. The 56 and 70 don't make this prefer that map size,
  // it's just that this algo was calibrated at that (standard)
  // map size, but it is intended to work for any size.
  CHECK_GT( map_sz.area(), 0 );
  double const scale_x =
      min( pow( 56.0 / map_sz.w, .125 ), 1.0 );
  double const scale_y =
      min( pow( 70.0 / map_sz.h, .125 ), 1.0 );

  double const dist_x =
      scale_x * dist_unscaled.w / max( map_sz.w / 2, 1 );
  double const dist_y =
      scale_y * dist_unscaled.h / max( map_sz.h / 2, 1 );

  double constexpr kExp = 6;
  dsize const sub{
    .w = pow( dist_x * edge_suppression.strength.w, kExp ),
    .h = pow( dist_y * edge_suppression.strength.h, kExp ),
  };
  level -= sub.pythagorean();
}

[[nodiscard]] double land_density_for_sea_level(
    matrix<double> const& m, double const sea_level ) {
  int land_count = 0;
  for( point const p : rect_iterator( m.rect() ) )
    if( m[p] > sea_level ) //
      ++land_count;
  return land_count * 1.0L / m.size().area();
}

[[nodiscard]] expect<double, e_perlin_map_error> find_sea_level(
    matrix<double> const& m, double const target_density ) {
  ScopedTimer const timer( "sea level search" );
  using enum e_sea_level;
  double const kIdealTolerance = 1e-4;
  double sea_level_min         = -1e6;
  double sea_level_max         = 1e6;
  double old_sea_level_min = -numeric_limits<double>::infinity();
  double old_sea_level_max = numeric_limits<double>::infinity();
  auto const sea_level_is  = [&]( double const sea_level,
                                  double const tolerance ) {
    double const density =
        land_density_for_sea_level( m, sea_level );
    lg.trace( "trying sea_level={} [{},{}] --> density={}",
              sea_level, sea_level_min, sea_level_max, density );
    double const target = target_density;
    if( abs( density - target ) < tolerance )
      return e_sea_level::good;
    if( density < target ) return e_sea_level::too_high;
    return e_sea_level::too_low;
  };
  lg.debug( "attempting to hit target land density: {}",
            target_density );
  double sea_level = {};
  int iters        = 0;
  for( iters = 0; iters < 10000; ++iters ) {
    sea_level = ( sea_level_min + sea_level_max ) / 2.0;
    switch( sea_level_is( sea_level, kIdealTolerance ) ) {
      case good:
        lg.debug( "perlin sea level bisections: {}", iters + 1 );
        return sea_level;
      case too_low:
        sea_level_min = sea_level;
        break;
      case too_high:
        sea_level_max = sea_level;
        break;
    }
    if( sea_level_min == old_sea_level_min &&
        sea_level_max == old_sea_level_max )
      break;
    old_sea_level_min = sea_level_min;
    old_sea_level_max = sea_level_max;
  }
  double fallback_tolerance = kIdealTolerance;
  // We want to allow up to .1, but compare a bit above that to
  // avoid rounding errors. Actually, a tolerance of .1 is
  // pretty bad... ideally we really don't want that, but there
  // are some small map sizes that seem to need this when tar-
  // getting certain densities, and we do want to try to flex-
  // ibly support small map sizes.
  while( fallback_tolerance < .1 + .000001 ) {
    if( sea_level_is( sea_level, fallback_tolerance ) ==
        e_sea_level::good ) {
      lg.warn(
          "land density did not meet ideal tolerance of {:.1} "
          "after sea level search of {} iterations, tolerance "
          "met: {:.1}",
          kIdealTolerance, iters + 1, fallback_tolerance );
      return sea_level;
    }
    fallback_tolerance *= 10;
  }
  return e_perlin_map_error::density_search_failed;
}

} // namespace

// The entropy object is itself sufficient to generate the num-
// bers that we want because it happens that the values that con-
// stitute the perlin seed fit within the 128 bits that are pro-
// vided by the rng::entropy type, so we can read the bits di-
// rectly instead of generating random numbers.
PerlinSeed generate_perlin_seed( rng::entropy e ) {
  // Make sure these get evaluated in order.
  uint32_t const offset_x = e.consume<uint32_t>();
  uint32_t const offset_y = e.consume<uint32_t>();
  uint32_t const base     = e.consume<uint32_t>();
  bool const flip         = e.consume<uint32_t>() % 2 == 0;
  return PerlinSeed{
    .offset_x = offset_x,
    .offset_y = offset_y,
    .base     = base,
    .flip     = flip,
  };
}

valid_or<e_perlin_map_error> land_gen_perlin(
    PerlinMapSettings const& settings,
    double const target_density, size const world_sz,
    matrix<e_surface>& out ) {
  using enum e_surface;
  size const sz = world_sz;
  lg.debug( "perlin: settings: {}", settings );
  lg.debug( "perlin: target_density: {}", target_density );
  lg.debug( "perlin: world_sz: {}", world_sz );
  if( sz.area() == 0 )
    return e_perlin_map_error::invalid_map_size;
  if( auto const ok = refl::validate_recursive( settings );
      !ok ) {
    lg.error( "invalid perlin settings: {}", ok.error() );
    return e_perlin_map_error::invalid_settings;
  }
  rng::vec2 const kNoRepeat{ .x = 100'000'000,
                             .y = 100'000'000 };
  // Repeat behavior of parameters:
  //   offset: repeats every kNumUniquePerlinHashes*scale.
  //   base:   repeats every kNumUniquePerlinHashes.
  auto const perlin_noise = [&]( point const p_real ) {
    rng::vec2 const p{
      .x = p_real.x * 1.0 + settings.seed.offset_x,
      .y = p_real.y * 1.0 + settings.seed.offset_y };
    double const noise = rng::perlin_noise_2d(
        p / settings.land_form.scale, settings.land_form.fractal,
        kNoRepeat, settings.seed.base );
    // NOTE: the range of these numbers will be roughly on the
    // order of [-1, 1], but could be larger or smaller in magni-
    // tude; the range isn't really constrained at this point.
    // Some perlin generators will normalize the result to fix
    // into [0,1] by rescaling. But we don't really need to do
    // that because we will be searching for the sea level using
    // a binary search below to achieve the target density, so
    // the range of the noise doesn't really matter. Also,
    // forcing it to fit into a fixed range makes the average
    // (which is otherwise ~0) to be very sensitive to the min
    // and max extremes, which might cause the edge suppression
    // mechanism to behave in a less predictable way (not sure
    // about that, but possible).
    return settings.seed.flip ? -noise : noise;
  };
  matrix<double> pm( sz );

  for( point const p : rect_iterator( pm.rect() ) )
    pm[p] = perlin_noise( p );

  if( settings.edge_suppression.enabled )
    for( point const p : rect_iterator( pm.rect() ) )
      perlin_suppress_edges( p, sz, settings.edge_suppression,
                             pm[p] );

  UNWRAP_RETURN_T( double const sea_level,
                   find_sea_level( pm, target_density ) );
  lg.debug( "sea_level: {}", sea_level );
  lg.info( "perlin land density: {:.3}",
           land_density_for_sea_level( pm, sea_level ) );

  out            = matrix<e_surface>( sz );
  int total_land = 0;
  for( point const p : rect_iterator( out.rect() ) ) {
    bool const is_land = pm[p] > sea_level;
    if( is_land ) ++total_land;
    out[p] = is_land ? land : water;
  }
  if( total_land == 0 && target_density > 0.0 )
    return e_perlin_map_error::density_too_small_no_land;

  CHECK( out.size().to_gfx() == world_sz );
  return valid;
}

} // namespace rn
