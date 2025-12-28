/****************************************************************
**perlin.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-24.
*
* Description: Perlin noise generator.
*
*****************************************************************/
#include "perlin.hpp"

// rng
#include "perlin-hashes.hpp"

// base
#include "base/error.hpp"

// C++ standard library
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>

namespace rng {

namespace {

using namespace std;

/****************************************************************
** Data.
*****************************************************************/
// Normalized unit vectors rotating clockwise.
PerlinFloat constexpr D = 0.70710678118; // 1/sqrt(2)
array<PerlinVec2, 8> constexpr kGradients2d{
  PerlinVec2{ 0, 1 },  PerlinVec2{ D, D },  PerlinVec2{ 1, 0 },
  PerlinVec2{ D, -D }, PerlinVec2{ 0, -1 }, PerlinVec2{ -D, -D },
  PerlinVec2{ -1, 0 }, PerlinVec2{ -D, D },
};

/****************************************************************
** Helpers.
*****************************************************************/
// Linear interpolation. Note that this can effectively be used
// to do smooth interpolation by just allowing the ramp parmeter
// to vary smoothly between the two points.
PerlinFloat lerp( PerlinFloat const ramp, PerlinFloat const a,
                  PerlinFloat const b ) {
  CHECK( ramp >= 0.0 && ramp <= 1.0 );
  return a + ramp * ( b - a );
}

[[nodiscard]] PerlinFloat dot( PerlinVec2 const p1,
                               PerlinVec2 const p2 ) {
  return p1.x * p2.x + p1.y * p2.y;
}

PerlinVec2 rand_gradient( PerlinInt const hash ) {
  PerlinInt const n = hash & 7;
  static_assert( kGradients2d.size() == 8 );
  return kGradients2d[n];
}

PerlinFloat dot_gradient( PerlinInt const hash,
                          PerlinVec2 const p ) {
  return dot( p, rand_gradient( hash ) );
}

/****************************************************************
** Single-octave Perlin noise.
*****************************************************************/
PerlinFloat perlin_noise_2d_single_octave(
    PerlinHashes const& hashes, PerlinVec2 p,
    PerlinVec2 const seamless_repeat, PerlinInt const base ) {
  auto const to_grid = [&]( PerlinVec2 const p ) {
    return pair{
      PerlinInt( floor( fmod( p.x, seamless_repeat.x ) ) ),
      PerlinInt( floor( fmod( p.y, seamless_repeat.y ) ) ),
    };
  };

  auto const hash = [&]( PerlinInt const i ) {
    return hashes[( i + base ) & kPerlinHashMask];
  };

  static auto const smooth_step = []( PerlinFloat const d ) {
    return d * d * d * ( d * ( d * 6 - 15 ) + 10 );
  };

  auto const [x1, y1] = to_grid( p );
  auto const [x2, y2] = to_grid( { x1 + 1.0, y1 + 1.0 } );

  PerlinInt const w  = hash( x1 );
  PerlinInt const e  = hash( x2 );
  PerlinInt const nw = hash( hash( w + y1 ) );
  PerlinInt const sw = hash( hash( w + y2 ) );
  PerlinInt const ne = hash( hash( e + y1 ) );
  PerlinInt const se = hash( hash( e + y2 ) );

  // Put p within [0,1.0) in each dimension, which puts it rela-
  // tive to the origin of its grid cell.
  p.x -= floor( p.x );
  p.y -= floor( p.y );

  auto const smooth_interp_x =
      bind_front( lerp, smooth_step( p.x ) );
  auto const smooth_interp_y =
      bind_front( lerp, smooth_step( p.y ) );

  // The 1's are subtracted to change the direction vector so
  // that it points from the relevant grid point to the sample
  // point.
  return smooth_interp_y(
      smooth_interp_x( dot_gradient( nw, p ),
                       dot_gradient( ne, { p.x - 1, p.y } ) ),
      smooth_interp_x(
          dot_gradient( sw, { p.x, p.y - 1 } ),
          dot_gradient( se, { p.x - 1, p.y - 1 } ) ) );
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
// Multi-octave perline noise.
PerlinFloat perlin_noise_2d(
    PerlinVec2 const point,
    PerlinFractalOptions const& fractal_options,
    PerlinVec2 const seamless_repeat, PerlinInt const base ) {
  if( fractal_options.n_octaves == 0 ) return 0.0;
  PerlinHashes const& hashes = perlin_hashes();

  PerlinFloat freq  = 1.0;
  PerlinFloat amp   = 1.0;
  PerlinFloat max   = 0.0;
  PerlinFloat total = 0.0;

  for( size_t i = 0; i < fractal_options.n_octaves; ++i ) {
    total += perlin_noise_2d_single_octave(
                 hashes, point * freq, seamless_repeat * freq,
                 base ) *
             amp;
    max += amp;
    freq *= fractal_options.lacunarity;
    amp *= fractal_options.persistence;
  }

  return total / max;
}

} // namespace rng
