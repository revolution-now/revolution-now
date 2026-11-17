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
#include "irand.hpp"
#include "terrain-mgr.hpp"

// ss
#include "ss/key.hpp"
#include "ss/map-matrix.hpp"

// C++ standard library
#include <ranges>

namespace rv = ::std::ranges::views;

namespace rn {

namespace {

using namespace std;

using ::base::valid;
using ::base::valid_or;
using ::gfx::point;
using ::rv::iota;

} // namespace

/****************************************************************
** TerrainGenerator.
*****************************************************************/
TerrainGenerator::TerrainGenerator(
    MapMatrix& m, IRand& rand, WeatherValue const& temperature,
    WeatherValue const& climate )
  : m_( m ),
    rand_( rand ),
    sz_( m.size() ),
    temperature_( temperature ),
    climate_( climate ),
    scratch_( sz_ ) {}

double TerrainGenerator::scale_h() const { return sz_.h / 70.0; }
double TerrainGenerator::scale_w() const { return sz_.w / 56.0; }
double TerrainGenerator::scale_area() const {
  return scale_h() * scale_w();
}

int TerrainGenerator::uniform( int const l, int const h ) const {
  return rand_.uniform_int( l, h );
}

bool TerrainGenerator::coin() const {
  return rand_.bernoulli( .5 );
}

valid_or<string> TerrainGenerator::full() {
  GOOD_OR_RETURN( temperature_pass() );
  GOOD_OR_RETURN( rainfall_pass_1() );
  GOOD_OR_RETURN( rainfall_pass_2() );
  GOOD_OR_RETURN( scatter_pass() );
  GOOD_OR_RETURN( forest_pass() );
  GOOD_OR_RETURN( rivers_pass() );
  GOOD_OR_RETURN( arctic_tundra_pass() );
  return valid;
}

[[nodiscard]] double incorrect_macro_abs( auto const& fn ) {
  return fn() > 0 ? fn() : -fn();
}

valid_or<string> TerrainGenerator::temperature_pass() {
  auto const band_to_biome = []( int const band ) {
    switch( band ) {
      using enum e_biome;
      case 0:
        return savannah;
      case 1:
        return grassland;
      case 2:
        return desert;
      case 3:
        return prairie;
      case 4:
      case 5:
        return plains;
      case 6:
      case 7:
      case 8:
      default:
        return tundra;
    }
  };

  // Number of temperature bands in on half of the map.
  double constexpr kNumBands = 9;
  double const band_size     = ( sz_.h / 2.0 ) / kNumBands;

  on_all( [&]( point const p, MapSquare& s ) {
    if( s.surface == e_surface::water ) return;
    auto const signed_equator_dist = [&] {
      double const equator      = sz_.h / 2.0;
      double const noise        = scale_h() * uniform( -8, 7 );
      double const rand_equator = equator + noise;
      return rand_equator - p.y;
    };
    double const equator_dist =
        incorrect_macro_abs( signed_equator_dist );
    // [-200,200] --> [-2,2] --> [2,-2].
    double const temperature_rescale =
        -temperature_.value / 100.0;
    // [-4,4].
    double const noise_with_temp = std::max(
        equator_dist + temperature_rescale * 2 * scale_h(),
        0.0 );
    int const band =
        static_cast<int>( noise_with_temp / band_size );

    s.ground = band_to_biome( band );
  } );

  return valid;
}

valid_or<string> TerrainGenerator::rainfall_pass_1() {
  double const climate_adjusted =
      ( climate_.value + 100 ) / 100.0;
  int const climate_adjusted_integral =
      static_cast<int>( round( climate_adjusted ) );
  for( int const y : iota( 0, sz_.h ) ) {
    int const equator_dist = abs( sz_.h / 2 - y );
    int const max_rainfall =
        std::max( 0, abs( sz_.h / 4 - equator_dist ) +
                         climate_adjusted_integral * 4 );
    int rainfall = uniform( 0, max_rainfall );
    for( int const x : iota( 0, sz_.w ) ) {
      point const p{ .x = x, .y = y };
      MapSquare& square = m_[p];
      Scratch& scratch  = scratch_[p];
      if( square.surface == e_surface::water ) {
        if( rainfall < max_rainfall ) ++rainfall;
      } else {
        using enum e_biome;
        if( square.overlay == e_land_overlay::mountains )
          rainfall -= 3;
        else if( square.overlay == e_land_overlay::hills )
          square.overlay.reset();
        else if( rainfall < 0 ) {
          switch( square.ground ) {
            case grassland:
              square.ground = prairie;
              break;
            case prairie:
              if( uniform( 0, abs( rainfall ) ) != 0 )
                square.ground = desert;
              else {
                square.ground = plains;
                --rainfall;
              }
              break;
            case plains:
              square.ground = tundra;
              break;
            case tundra:
              scratch.forest_plan = e_forest_plan::suppress;
              break;
            case arctic:
            case desert:
            case marsh:
            case savannah:
            case swamp:
              break;
          }
        } else if( rainfall > 0 ) {
          switch( square.ground ) {
            case savannah:
              rainfall -= 2;
              if( uniform( 0, 3 ) == 0 ) square.ground = swamp;
              break;
            case grassland:
              rainfall -= 2;
              if( uniform( 0, 3 ) == 0 ) square.ground = marsh;
              break;
            case prairie:
              square.ground = grassland;
              break;
            case plains:
              square.ground = prairie;
              break;
            case tundra:
              square.ground = plains;
              break;
            case arctic:
            case desert:
            case marsh:
            case swamp:
              break;
          }
        }
        if( rainfall > 0 )
          rainfall -= uniform(
              1,
              std::max( 1, 7 - climate_adjusted_integral * 2 ) );
        else if( rainfall < 0 )
          ++rainfall;
      }
    }
  }
  return valid;
}

valid_or<string> TerrainGenerator::rainfall_pass_2() {
  double const climate_adjusted =
      ( climate_.value + 100 ) / 100.0;
  int const climate_adjusted_integral =
      static_cast<int>( round( climate_adjusted ) );
  for( int const y : iota( 0, sz_.h ) ) {
    int const equator_dist = abs( sz_.h / 2 - y );
    // NOTE: this could be negative, but that's probably fine.
    int const max_rainfall =
        equator_dist / 2 + climate_adjusted_integral;
    int rainfall = 0;
    for( int x = sz_.w - 1; x >= 0; --x ) {
      point const p{ .x = x, .y = y };
      MapSquare& square = m_[p];
      if( square.surface == e_surface::water ) {
        if( rainfall < max_rainfall ) ++rainfall;
      } else {
        if( square.overlay == e_land_overlay::mountains )
          rainfall -= 3;
        else if( square.overlay == e_land_overlay::hills )
          square.overlay.reset();
        else if( rainfall < 0 ) {
          // None.
        } else if( rainfall > 0 ) {
          switch( square.ground ) {
            using enum e_biome;
            case savannah:
              rainfall -= 2;
              square.ground = swamp;
              break;
            case grassland:
              rainfall -= 2;
              if( coin() ) square.ground = marsh;
              break;
            case prairie:
              square.ground = grassland;
              break;
            case plains:
              square.ground = prairie;
              break;
            case desert:
              square.ground = prairie;
              break;
            case tundra:
              square.ground = plains;
              break;
            case arctic:
            case marsh:
            case swamp:
              break;
          }
        }
        if( rainfall > 0 )
          rainfall -= uniform(
              1,
              std::max( 1, 7 - climate_adjusted_integral * 2 ) );
        else if( rainfall < 0 )
          ++rainfall;
      }
    }
  }
  return valid;
}

bool TerrainGenerator::ocean( point const p ) const {
  if( !m_.exists( p ) ) return false;
  if( p.x == 0 || p.y == 0 ) return false;
  if( p.x == sz_.w - 1 || p.y == sz_.h - 1 ) return false;
  using enum e_surface;
  if( m_[p.moved( e_direction::nw )].surface == water )
    return false;
  if( m_[p.moved( e_direction::sw )].surface == water )
    return false;
  if( m_[p.moved( e_direction::ne )].surface == water )
    return false;
  if( m_[p.moved( e_direction::se )].surface == water )
    return false;
  return true;
}

void TerrainGenerator::enable_hills( point const p ) {
  MapSquare& square = m_[p];
  if( !square.overlay.has_value() ) {
    square.overlay = e_land_overlay::hills;
    return;
  }
  switch( *square.overlay ) {
    case rn::e_land_overlay::forest: {
      square.overlay = e_land_overlay::hills;
      break;
    }
    case rn::e_land_overlay::hills:
      break;
    case rn::e_land_overlay::mountains:
      // Don't change mountains.
      break;
  }
}

void TerrainGenerator::enable_mountains( point const p ) {
  MapSquare& square = m_[p];
  if( !square.overlay.has_value() ) {
    square.overlay = e_land_overlay::mountains;
    return;
  }
  switch( ( *square.overlay ) ) {
    case rn::e_land_overlay::forest: {
      square.overlay = e_land_overlay::mountains;
      break;
    }
    case rn::e_land_overlay::hills:
      square.overlay = e_land_overlay::mountains;
      break;
    case rn::e_land_overlay::mountains:
      break;
  }
}

valid_or<string> TerrainGenerator::scatter_pass() {
  int x = 0, y = 0;
  int const g_dir_dx[9] = { 0, 1, 1, 1, 0, -1, -1, -1, 0 };
  int const g_dir_dy[9] = { -1, -1, 0, 1, 1, 1, 0, -1, 0 };
  int pa = 0, pb = 0; // FIXME
  for( size_t const n : iota( 0, 800 ) ) {
    if( ( n & 1 ) != 0 ) {
      int const r = uniform( 0, 8 );
      x += g_dir_dx[r];
      y += g_dir_dy[r];
    } else {
      // This point can't be on the border because the next pass
      // will try to take a point next to it.
      x = uniform( 1, sz_.w - 2 );
      y = uniform( 1, sz_.h - 2 );
    }
    point const p{ .x = x, .y = y };
    MapSquare& square = m_[p];
    Scratch& scratch  = scratch_[p];
    if( square.overlay == e_land_overlay::mountains ) {
      if( ocean( p ) ) square.overlay.reset();
    } else if( square.overlay == e_land_overlay::hills ) {
      square.overlay      = e_land_overlay::mountains;
      scratch.forest_plan = e_forest_plan::populate;
    } else {
      pa = 0;
      pb = 0;
      switch( square.ground ) {
        using enum e_biome;
        case tundra:
          pa = 1;
          pb = 0;
          if( coin() ) square.ground = plains;
          break;
        case desert:
          pa = 1;
          pb = 1;
          if( coin() ) square.ground = prairie;
          break;
        case plains:
        case prairie:
          if( square.ground == prairie && uniform( 0, 2 ) == 0 )
            square.ground = plains;
          pa = pb = 2;
          if( coin() )
            scratch.forest_plan = e_forest_plan::suppress;
          break;
        case grassland:
          pa = 3;
          pb = 1;
          if( coin() ) square.ground = marsh;
          if( coin() )
            scratch.forest_plan = e_forest_plan::populate;
          break;
        case savannah:
          pa = 3;
          pb = 2;
          if( coin() ) square.ground = swamp;
          if( coin() )
            scratch.forest_plan = e_forest_plan::populate;
          break;
        case marsh:
          pa = 5;
          pb = 3;
          if( coin() ) square.ground = grassland;
          if( coin() )
            scratch.forest_plan = e_forest_plan::suppress;
          break;
        case swamp:
          pa = 5;
          pb = 3;
          if( coin() ) square.ground = savannah;
          if( coin() )
            scratch.forest_plan = e_forest_plan::suppress;
          break;
        case arctic:
          break;
      }
    }
    if( pa != 0 && uniform( 0, pa ) == 0 ) {
      enable_hills( p );
      if( pb != 0 && uniform( 0, pb ) == 0 )
        enable_mountains( p );
    }
  }
  return valid;
}

bool TerrainGenerator::ocean_adjacent( point const p ) const {
  bool has = false;
  on_surrounding( m_, p,
                  [&]( point, MapSquare const& adjacent ) {
                    if( adjacent.surface == e_surface::water )
                      has = true;
                  } );
  return has;
}

valid_or<string> TerrainGenerator::forest_pass() {
  for( int const y : iota( 0, sz_.h ) ) {
    for( int const x : iota( 0, sz_.w ) ) {
      point const p{ .x = x, .y = y };
      MapSquare& square = m_[p];
      Scratch& scratch  = scratch_[p];
      if( square.surface == e_surface::water ) continue;
      if( square.overlay.has_value() ) continue;
      switch( scratch.forest_plan ) {
        using enum e_forest_plan;
        case populate:
          square.overlay = e_land_overlay::forest;
          break;
        case deferred:
        case suppress:
          if( ocean_adjacent( p ) ) {
            if( coin() )
              square.overlay = e_land_overlay::forest;
            else if( uniform( 0, 4 ) != 0 )
              square.overlay = e_land_overlay::forest;
          }
          break;
      }
    }
  }
  return valid;
}

valid_or<string> TerrainGenerator::rivers_pass() {
  return valid;
}

valid_or<string> TerrainGenerator::arctic_tundra_pass() {
  using enum e_surface;
  using enum e_biome;
  point p;
  auto const set_cold = [&]( e_biome const what ) {
    MapSquare& square = m_[p];
    if( square.surface != land ) return;
    square.ground = what;
    square.overlay.reset();
  };
  for( bool const top : { true, false } ) {
    for( int const x : iota( 0, sz_.w ) ) {
      p.x = x;

      // Outer-most row: all arctic.
      p.y = top ? 0 : sz_.h - 1;
      set_cold( arctic );

      // Second row: half arctic, half tundra.
      p.y = top ? 1 : sz_.h - 2;
      set_cold( coin() ? arctic : tundra );

      // Third row: half tundra.
      p.y = top ? 2 : sz_.h - 3;
      if( coin() ) set_cold( tundra );
    }
  }
  return valid;
}

} // namespace rn
