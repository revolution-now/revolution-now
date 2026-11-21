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

// base
#include "base/logger.hpp"

// C++ standard library
#include <ranges>

namespace rv = ::std::ranges::views;

namespace rn {

namespace {

using namespace std;

using ::base::valid;
using ::base::valid_or;
using ::gfx::e_side;
using ::gfx::point;
using ::gfx::size;
using ::refl::enum_values;
using ::rv::iota;

[[nodiscard]] double compute_density( MapMatrix const& m ) {
  double density = 0.0;
  on_all_tiles( m, [&]( point const _, MapSquare const& s ) {
    if( s.surface == e_surface::water ) return;
    density += 1.0;
  } );
  density /= m.size().area();
  return density;
}

} // namespace

/****************************************************************
** TerrainGenerator.
*****************************************************************/
TerrainGenerator::TerrainGenerator(
    MapMatrix& m, IRand& rand, double const scale,
    WeatherValue const& temperature,
    WeatherValue const& climate )
  : m_( m ),
    rand_( rand ),
    sz_( m.size() ),
    density_( compute_density( m_ ) ),
    scale_( scale ),
    temperature_( temperature ),
    climate_( climate ),
    scratch_( sz_ ) {}

double TerrainGenerator::scale_h() const { return sz_.h / 70.0; }
double TerrainGenerator::scale_w() const { return sz_.w / 56.0; }
double TerrainGenerator::scale_area() const {
  return scale_h() * scale_w();
}
double TerrainGenerator::scale_linear() const {
  return ( sz_.h + sz_.w ) / ( 70.0 + 56.0 );
}

int TerrainGenerator::uniform( int const l, int const h ) const {
  return rand_.uniform_int( l, h );
}

bool TerrainGenerator::coin() const {
  return rand_.bernoulli( .5 );
}

valid_or<string> TerrainGenerator::full() {
  GOOD_OR_RETURN( temperature_pass() );
  GOOD_OR_RETURN( elevate() );
  GOOD_OR_RETURN( rainfall_pass_1() );
  GOOD_OR_RETURN( rainfall_pass_2() );
  GOOD_OR_RETURN( scatter_pass() );
  GOOD_OR_RETURN( forest_pass() );
  GOOD_OR_RETURN( rivers_pass() );
  GOOD_OR_RETURN( arctic_tundra_pass() );
  return valid;
}

[[nodiscard]] double incorrect_macro_abs( auto const& fn ) {
  // This is generally incorrect because the function fn is not
  // guaranteed to return the same result on each call (e.g. it
  // could be random), and so one should not call the function
  // again in the two branches. However, this appears to have
  // been a bug in the OG when computing abs in some cases, and
  // it has a noticeable effect on the map, and the map gen algo
  // was probably tuned with this bug in place, and so we will
  // preserve it.
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
    // Using this "incorrect" abs as opposed to a normal abs()
    // has the effect of increasing the amount of savannah around
    // the equator.
    double const equator_dist =
        incorrect_macro_abs( signed_equator_dist );
    double const temperature_rescale =
        -temperature_.value / 100.0;
    double const noise_with_temp = std::max(
        equator_dist + temperature_rescale * 2 * scale_h(),
        0.0 );
    int const band =
        static_cast<int>( noise_with_temp / band_size );

    s.ground = band_to_biome( band );
  } );

  return valid;
}

valid_or<string> TerrainGenerator::elevate() {
  double density = 0.0;
  on_all( [&]( point const _, MapSquare& s ) {
    if( s.surface == e_surface::water ) return;
    density += 1.0;
  } );
  density /= m_.size().area();
  // This makes scale on mode m have 0, and each mode adds or
  // subtracts 1.0 because in our config we use increments of 3.0
  // for scale.
  double const normalized_scale = ( scale_ - 12.0 ) / 3.0;
  // This makes density on mode m have 0, and each mode adds or
  // subtracts 1.0 because in our config we use increments of .07
  // for land mass.
  double const normalized_density = ( density - .23 ) / .07;
  double const normalized_sum =
      normalized_scale + normalized_density;
  double const kProbMountains =
      std::max( 0.0, ( .005 * normalized_sum ) + .01 );
  double const kProbHills =
      std::max( 0.0, ( .05 * normalized_sum ) + .20 );
#if 0
  [[maybe_unused]] static auto _ = [&] {
    fmt::println( "density:            {}", density );
    fmt::println( "normalized_scale:   {}", normalized_scale );
    fmt::println( "normalized_density: {}", normalized_density );
    fmt::println( "normalized_sum:     {}", normalized_sum );
    fmt::println( "kProbMountains:     {}", kProbMountains );
    fmt::println( "kProbHills:         {}", kProbHills );
    return true;
  }();
#endif
  on_all( [&]( point const _, MapSquare& s ) {
    if( s.surface == e_surface::water ) return;
    double const d = rand_.uniform_double( 0, 1.0 );
    if( d < kProbMountains )
      s.overlay = e_land_overlay::mountains;
    else if( d < kProbMountains + kProbHills )
      s.overlay = e_land_overlay::hills;
  } );
  return valid;
}

valid_or<string> TerrainGenerator::rainfall_pass_1() {
  double const climate_adjusted =
      ( climate_.value + 100 ) / 100.0;
  for( int const y : iota( 0, sz_.h ) ) {
    double const equator_dist = abs( sz_.h / 2.0 - y );
    int const max_rainfall    = static_cast<int>( std::max(
        0.0, abs( ( sz_.h / 4.0 - equator_dist ) / scale_h() ) +
                 climate_adjusted * 4 ) );
    int rainfall              = uniform( 0, max_rainfall );
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
              std::max( 1, static_cast<int>( round(
                               7 - climate_adjusted * 2 ) ) ) );
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
  for( int const y : iota( 0, sz_.h ) ) {
    double const equator_dist = abs( sz_.h / 2.0 - y );
    // NOTE: this could be negative, but that's probably fine.
    int const max_rainfall = static_cast<int>(
        ( equator_dist / scale_h() ) / 2 + climate_adjusted );
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
              std::max( 1, static_cast<int>( round(
                               7 - climate_adjusted * 2 ) ) ) );
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
  int const iters =
      static_cast<int>( round( 800 * scale_area() ) );
  CHECK_GE( iters, 0 );
  lg.debug( "iterations in biome scatter pass: {}", iters );
  for( size_t const n : iota( 0, iters ) ) {
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
      int hills     = 0;
      int mountains = 0;
      switch( square.ground ) {
        using enum e_biome;
        case tundra:
          hills     = 1;
          mountains = 0;
          if( coin() ) square.ground = plains;
          break;
        case desert:
          hills     = 1;
          mountains = 1;
          if( coin() ) square.ground = prairie;
          break;
        case plains:
        case prairie:
          if( square.ground == prairie && uniform( 0, 2 ) == 0 )
            square.ground = plains;
          hills = mountains = 2;
          if( coin() )
            scratch.forest_plan = e_forest_plan::suppress;
          break;
        case grassland:
          hills     = 3;
          mountains = 1;
          if( coin() ) square.ground = marsh;
          if( coin() )
            scratch.forest_plan = e_forest_plan::populate;
          break;
        case savannah:
          hills     = 3;
          mountains = 2;
          if( coin() ) square.ground = swamp;
          if( coin() )
            scratch.forest_plan = e_forest_plan::populate;
          break;
        case marsh:
          hills     = 5;
          mountains = 3;
          if( coin() ) square.ground = grassland;
          if( coin() )
            scratch.forest_plan = e_forest_plan::suppress;
          break;
        case swamp:
          hills     = 5;
          mountains = 3;
          if( coin() ) square.ground = savannah;
          if( coin() )
            scratch.forest_plan = e_forest_plan::suppress;
          break;
        case arctic:
          break;
      }
      if( hills != 0 && uniform( 0, hills ) == 0 ) {
        enable_hills( p );
        if( mountains != 0 && uniform( 0, mountains ) == 0 )
          enable_mountains( p );
      }
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
  double const climate_adjusted =
      ( climate_.value + 100 ) / 100.0;
  int const climate_adjusted_integral =
      static_cast<int>( round( climate_adjusted ) );
  int rivers = 0;
  int tries  = 0;
  // We ultimately don't use a "land mass" to generate land like
  // the OG does; instead we use a density. This will roughly
  // translate between our density and the OG's customization op-
  // tion 0 ("land mass") which goes from [0,2] (though in our
  // case it can extend beyond those boundaries).
  double const og_land_mass = ( ( density_ - .23 ) / .07 ) + 1.0;
  int const target_count    = static_cast<int>( round(
      scale_area() *
      ( ( og_land_mass + climate_adjusted ) * 4 + 8 ) * 2 ) );
  int const max_tries =
      static_cast<int>( round( 512 * scale_area() ) );
  vector<point> river;
  do {
    river.clear();
    ++tries;
    int len = 0;
    point p;
    MapSquare* square = {};
    do {
      p.x    = uniform( 0, sz_.w - 1 );
      p.y    = uniform( 0, sz_.h - 1 );
      square = &m_[p];
    } while( square->surface == e_surface::water ||
             square->overlay == e_land_overlay::mountains ||
             square->overlay == e_land_overlay::hills );
    e_cardinal_direction dir =
        rand_.pick_one( enum_values<e_cardinal_direction> );
    e_side turn = rand_.pick_one( enum_values<e_side> );
    bool joined = false;
    do {
      river.push_back( p );
      ++len;
      for( auto const d : enum_values<e_cardinal_direction> ) {
        if( joined ) break;
        point const moved = p.moved( d );
        if( !m_.exists( moved ) ) continue;
        MapSquare& square_moved = m_[moved];
        if( square_moved.surface == e_surface::water ||
            square_moved.river.has_value() ) {
          joined = true;
          if( !square_moved.river.has_value() )
            // ocean.
            square_moved.river = e_river::minor;
        }
      }
      if( joined ) break;
      int const r = uniform( 0, 99 );
      if( r < 60 )
        // No direction change
        ;
      else {
        if( r > 95 ) turn = reverse( turn );
        switch( turn ) {
          case e_side::left:
            dir = rotated_ccw( dir );
            break;
          case e_side::right:
            dir = rotated_cw( dir );
            break;
        }
        turn = reverse( turn );
      }
      p = p.moved( dir );
      if( !m_.exists( p ) ) break;
      if( m_[p].overlay == e_land_overlay::hills ) break;
      if( m_[p].overlay == e_land_overlay::mountains ) break;
      CHECK( !m_[p].river.has_value() );
    } while( true );
    if( !joined ) continue;
    if( !m_.exists( p ) ) continue;
    if( len < 3 ) continue;
    ++rivers;
    int len2 = 0;
    if( uniform( 1, climate_adjusted_integral * 2 + 12 ) > 6 ) {
      len2 = uniform(
          1, std::max( 1, static_cast<int>( round(
                              ( climate_adjusted * 2 + 3 ) *
                              scale_linear() ) ) ) );
      CHECK( !river.empty() );
      CHECK( m_.exists( river.back() ) );
      for( point const& p :
           rv::take( rv::reverse( river ), len2 ) )
        m_[p].river = e_river::major;
    }
    static size const ring[] = {
      // clang-format off
                    {.w=-1,.h=-2},{.w=0,.h=-2},{.w=1,.h=-2},
      {.w=-2,.h=-1},{.w=-1,.h=-1},{.w=0,.h=-1},{.w=1,.h=-1},{.w=2,.h=-1},
      {.w=-2,.h= 0},{.w=-1,.h= 0},             {.w=1,.h= 0},{.w=2,.h= 0},
      {.w=-2,.h= 1},{.w=-1,.h= 1},{.w=0,.h= 1},{.w=1,.h= 1},{.w=2,.h= 1},
                    {.w=-1,.h= 2},{.w=0,.h= 2},{.w=1,.h= 2},
      // clang-format on
    };
    CHECK( !river.empty() );
    CHECK( m_.exists( river.back() ) );
    for( size const delta : ring ) {
      point const moved = river.back() + delta;
      if( !m_.exists( moved ) ) continue;
      MapSquare& square_moved = m_[moved];
      if( square_moved.surface == e_surface::water ) continue;
      if( !square_moved.overlay.has_value() )
        square_moved.overlay = e_land_overlay::forest;
    }
  } while( tries < max_tries && rivers < target_count );
  return valid;
}

// Note that we opt not to scale this with map height, since the
// arctic is kind of a special thing that should only occupy one
// line in order to preserve the look of the OG.
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
