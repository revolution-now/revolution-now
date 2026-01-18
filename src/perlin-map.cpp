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
#include "rand/perlin.hpp"

// base
#include "base/logger.hpp"
#include "base/timer.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <algorithm>

using namespace std;

namespace rn {

namespace {

using ::base::ScopedTimer;
using ::gfx::dsize;
using ::gfx::Matrix;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::rect_iterator;
using ::gfx::size;
using ::std::max;
using ::std::min;

// This edge-suppression algo only makes sense with the Perlin
// generator because it works by lowering the "height" of the
// tiles near the edges, making it more likely that they will end
// up under the "sea level".
//
// NOTE: even though we use the classic map size dimensions as a
// reference in this function, it is designed to work for any map
// size, even super small or super large.
void perlin_suppress_edges( point const p, size const map_sz,
                            double const suppression_decay,
                            double& level ) {
  int const dist_x_unscaled =
      ( p.x < map_sz.w / 2 ) ? p.x : map_sz.w - p.x - 1;
  int const dist_y_unscaled =
      ( p.y < map_sz.h / 2 ) ? p.y : map_sz.h - p.y - 1;

  dsize const scale{
    .w = sqrt( map_sz.w / 56.0 ),
    .h = sqrt( map_sz.h / 70.0 ),
  };

  // x2 for y for less suppression near top/bottom edges.
  int const dist_x = lround( dist_x_unscaled / scale.w );
  int const dist_y = lround( dist_y_unscaled / scale.h ) * 2;

  int const dist = min( dist_x, dist_y );

  double sub = 2.0;
  for( int i = 0; i < dist; ++i ) sub *= suppression_decay;
  level -= sub;
}

} // namespace

void land_gen_perlin( PerlinMapSettings const& settings,
                      double const target_density,
                      size const world_sz,
                      Matrix<e_surface>& surface ) {
  size const sz = world_sz;
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
    // the range of the noise doesn't really matter.
    return noise;
  };
  Matrix<double> pm( sz );

  auto const print_stats = [&]( string_view const name ) {
#if 0
    double max_val  = 0.0;
    double min_val  = numeric_limits<double>::max();
    double avg      = 0;
    double variance = 0;
    for( point const p : rect_iterator( pm.rect() ) ) {
      double const val = pm[p];
      if( val > max_val ) max_val = val;
      if( val < min_val ) min_val = val;
      avg += val;
      variance += val * val;
    }
    avg /= pm.size().area();
    double const stddev = sqrt( variance - avg * avg );
    lg.info(
        "{}: min_val={:.3}, max_val={:.3}, avg={:.3}, "
        "stddev={:.3}",
        name, min_val, max_val, avg, stddev );
#else
    (void)name;
#endif
  };

  print_stats( "1" );

  {
    ScopedTimer const timer( "noise generation" );
    for( point const p : rect_iterator( pm.rect() ) )
      pm[p] = perlin_noise( p );
  }

  print_stats( "2" );

  if( settings.edge_suppression.enabled ) {
    ScopedTimer const timer( "edge suppression" );
    for( point const p : rect_iterator( pm.rect() ) )
      perlin_suppress_edges(
          p, sz, settings.edge_suppression.suppression_decay,
          pm[p] );
  }

  print_stats( "3" );

  double const kTolerance = 1e-3;
  auto const land_density = [&]( double const sea_level ) {
    int land_count = 0;
    for( point const p : rect_iterator( pm.rect() ) )
      if( pm[p] > sea_level ) //
        ++land_count;
    return land_count * 1.0 / pm.size().area();
  };
  enum class e_sea_level {
    too_low,
    good,
    too_high,
  };
  double sea_level_min    = -10.0;
  double sea_level_max    = 10.0;
  auto const sea_level_is = [&]( double const sea_level ) {
    double const density = land_density( sea_level );
    lg.trace( "trying sea_level={} [{},{}] --> density={}",
              sea_level, sea_level_min, sea_level_max, density );
    double const target = target_density;
    if( abs( density - target ) < kTolerance )
      return e_sea_level::good;
    if( density < target ) return e_sea_level::too_high;
    return e_sea_level::too_low;
  };
  [&] {
    ScopedTimer const timer( "sea level search" );
    using enum e_sea_level;
    for( int i = 0; i < 100; ++i ) {
      double const sea_level =
          ( sea_level_min + sea_level_max ) / 2.0;
      switch( sea_level_is( sea_level ) ) {
        case good:
          lg.info( "perlin sea level bisections: {}", i + 1 );
          return;
        case too_low:
          sea_level_min = sea_level;
          break;
        case too_high:
          sea_level_max = sea_level;
          break;
      }
    }
  }();
  double const sea_level =
      ( sea_level_min + sea_level_max ) / 2.0;
  lg.info( "sea_level: {}", sea_level );

  print_stats( "4" );

  lg.info( "perlin land density: {:.3}",
           land_density( sea_level ) );

  {
    using enum e_surface;
    auto& m = surface;
    m       = Matrix<e_surface>( sz );
    ScopedTimer const timer( "final map build" );
    for( point const p : rect_iterator( m.rect() ) )
      m[p] = ( pm[p] <= sea_level ) ? water : land;
  }

  print_stats( "5" );
}

} // namespace rn
