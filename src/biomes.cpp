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

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/logger.hpp"

// C++ standard library
#include <ranges>

namespace rv = ::std::ranges::views;

namespace rn {

namespace {

using namespace std;

using ::base::function_ref;
using ::base::maybe;
using ::base::nothing;
using ::base::valid;
using ::base::valid_or;
using ::gfx::e_cardinal_cdirection;
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

template<same_as<double> T>
[[nodiscard]] int rounded( T const d ) {
  return static_cast<int>( std::round( d ) );
}

} // namespace

/****************************************************************
** TerrainGenerator.
*****************************************************************/
TerrainGenerator::TerrainGenerator(
    MapMatrix& m, IRand& rand, GeneratedMapSetup const& setup )
  : m_( m ),
    rand_( rand ),
    setup_( setup ),
    sz_( m.size() ),
    density_( compute_density( m_ ) ),
    scale_( setup.surface_generator.perlin_settings.land_form
                .scale ),
    temperature_( setup.weather.temperature ),
    climate_( setup.weather.climate ),
    scratch_( sz_ ) {
  CHECK_EQ( m.size(), setup.size );
}

double TerrainGenerator::scale_h() const { return sz_.h / 70.0; }
double TerrainGenerator::scale_w() const { return sz_.w / 56.0; }
double TerrainGenerator::scale_area() const {
  return scale_h() * scale_w();
}
double TerrainGenerator::scale_linear() const {
  return ( sz_.h + sz_.w ) / ( 70.0 + 56.0 );
}

maybe<point> TerrainGenerator::random_tile(
    int const max_tries,
    function_ref<bool( point, MapSquare const& ) const> pred )
    const {
  int tries = 0;
  while( tries++ < max_tries ) {
    point p;
    p.x = uniform( 0, sz_.w - 1 );
    p.y = uniform( 0, sz_.h - 1 );
    if( pred( p, m_[p] ) ) return p;
  }
  return nothing;
}

int TerrainGenerator::uniform( int const l, int const h ) const {
  return rand_.uniform_int( l, h );
}

int TerrainGenerator::uniform_from_0_to( int const n ) const {
  int upper = n;
  upper     = std::max( 0, upper );
  return uniform( 0, upper );
}

int TerrainGenerator::uniform_from_0_to( double const d ) const {
  int upper = rounded( d );
  upper     = std::max( 0, upper );
  return uniform( 0, upper );
}

int TerrainGenerator::uniform_from_1_to( int const n ) const {
  int upper = n;
  upper     = std::max( 1, upper );
  return uniform( 1, upper );
}

int TerrainGenerator::uniform_from_1_to( double const d ) const {
  int upper = rounded( d );
  upper     = std::max( 1, upper );
  return uniform( 1, upper );
}

bool TerrainGenerator::coin() const {
  return rand_.bernoulli( .5 );
}

valid_or<string> TerrainGenerator::full() {
  rand_.reseed( setup_.elevation.seed );
  GOOD_OR_RETURN( elevate() );

  rand_.reseed( setup_.biomes.seed );
  GOOD_OR_RETURN( temperature_pass() );
  GOOD_OR_RETURN( rainfall_pass_1() );
  GOOD_OR_RETURN( rainfall_pass_2() );
  GOOD_OR_RETURN( scatter_pass() );
  GOOD_OR_RETURN( forest_pass() );
  if( setup_.surface_generator.arctic.enabled )
    GOOD_OR_RETURN( arctic_tundra_pass() );

  if( setup_.rivers.has_value() ) {
    rand_.reseed( setup_.rivers->seed );
    GOOD_OR_RETURN( rivers_pass( setup_.rivers->parameters ) );
  }

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
    double const noise_with_temp =
        std::max( equator_dist +
                      ( 1.0 - og_temperature() ) * 2 * scale_h(),
                  0.0 );
    int const band =
        static_cast<int>( noise_with_temp / band_size );

    s.ground = band_to_biome( band );
  } );

  return valid;
}

valid_or<string> TerrainGenerator::elevate() {
  double const kProbMountains =
      std::max( density_ * scale_ * .01 - .017, 0.0 );
  double const kProbHills = .25;
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
  for( int const y : iota( 0, sz_.h ) ) {
    double const equator_dist = abs( sz_.h / 2.0 - y );
    int const max_rainfall    = static_cast<int>( std::max(
        0.0, abs( ( sz_.h / 4.0 - equator_dist ) / scale_h() ) +
                 og_climate() * 4 ) );
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
              std::max( 1, rounded( 7 - og_climate() * 2 ) ) );
        else if( rainfall < 0 )
          ++rainfall;
      }
    }
  }
  return valid;
}

valid_or<string> TerrainGenerator::rainfall_pass_2() {
  for( int const y : iota( 0, sz_.h ) ) {
    double const equator_dist = abs( sz_.h / 2.0 - y );
    // NOTE: this could be negative, but that's probably fine.
    int const max_rainfall = static_cast<int>(
        ( equator_dist / scale_h() ) / 2 + og_climate() );
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
              std::max( 1, rounded( 7 - og_climate() * 2 ) ) );
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
  // In the OG, the hills representation is a subset of the moun-
  // tains representation, so when hills are added to a mountains
  // tile it has no effect.
  if( m_[p].overlay == e_land_overlay::mountains ) return;
  m_[p].overlay = e_land_overlay::hills;
}

void TerrainGenerator::enable_mountains( point const p ) {
  m_[p].overlay = e_land_overlay::mountains;
}

valid_or<string> TerrainGenerator::scatter_pass() {
  point p;
  int const world_age = 1;
  int const og_iters  = world_age * 800 + 800;
  int const iters     = rounded( og_iters * scale_area() );
  CHECK_GE( iters, 0 );
  lg.debug( "iterations in biome scatter pass: {}", iters );
  for( size_t const n : iota( 0, iters ) ) {
    if( n % 2 == 1 ) {
      p = p.moved( rand_.pick_enum<e_cardinal_cdirection>() );
    } else {
      // This point can't be on the border because the next pass
      // will try to take a point next to it.
      p.x = uniform( 1, sz_.w - 2 );
      p.y = uniform( 1, sz_.h - 2 );
    }
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

double const kDensityM = .23;
double const kScaleM   = 12;

double TerrainGenerator::og_land_mass() const {
  return 1.0 + 24 * ( density_ - kDensityM ) -
         .38 * ( scale_ - kScaleM );
}

double TerrainGenerator::og_land_form() const {
  return 1.0 + 13 * ( density_ - kDensityM ) -
         .49 * ( scale_ - kScaleM );
}

double TerrainGenerator::og_temperature() const {
  return ( temperature_.value + 100 ) / 100.0;
}

double TerrainGenerator::og_climate() const {
  return ( climate_.value + 100 ) / 100.0;
}

int TerrainGenerator::og_land_mass_int() const {
  return rounded( og_land_mass() );
}

int TerrainGenerator::og_land_form_int() const {
  return rounded( og_land_form() );
}

int TerrainGenerator::og_temperature_int() const {
  return rounded( og_temperature() );
}

int TerrainGenerator::og_climate_int() const {
  return rounded( og_climate() );
}

valid_or<string> TerrainGenerator::rivers_pass(
    RiverParameters const& params ) {
  int const target_count = std::max(
      rounded( ( ( og_land_mass() + og_climate() ) * 4 + 8 ) *
               2 * scale_area() * params.density.value ),
      0 );
  // If the density is larger than 1.0 then that means we are
  // asking for more rivers than the OG, so be sure to allow more
  // tries to fulfill that request.
  int const max_tries =
      rounded( 512 * scale_area() *
               std::max( params.density.value, 1.0 ) );
  auto const on_arctic = [&]( point const p ) {
    return p.y == 0 || p.y == sz_.h - 1;
  };
  auto const allowed_land_for_river =
      [&]( point const p, MapSquare const& square ) {
        return square.surface != e_surface::water &&
               square.overlay != e_land_overlay::mountains &&
               square.overlay != e_land_overlay::hills &&
               !on_arctic( p );
      };
  int rivers = 0;
  int tries  = 0;
  vector<point> river;
  while( tries < max_tries && rivers < target_count ) {
    river.clear();
    ++tries;
    auto const maybe_p =
        random_tile( sz_.area(), allowed_land_for_river );
    if( !maybe_p.has_value() )
      // Probably won't do an error here, since it can happen.
      break;
    point p = *maybe_p;
    e_cardinal_direction dir =
        rand_.pick_enum<e_cardinal_direction>();
    e_side turn = rand_.pick_enum<e_side>();
    while( true ) {
      river.push_back( p );
      for( auto const d : enum_values<e_cardinal_direction> ) {
        point const moved = p.moved( d );
        if( !m_.exists( moved ) ) continue;
        auto& square_moved = m_[moved];
        if( square_moved.surface == e_surface::water ||
            square_moved.river.has_value() ) {
          // There may already be a river here but that's ok.
          river.push_back( moved );
          goto joined;
        }
      }
      int const turn_threshold =
          rounded( params.turning_frequency.probability * 100 );
      if( int const r = uniform_from_0_to( 99 );
          r >= turn_threshold ) {
        // Direction change.
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
      // If this new point that we've just moved to either has a
      // river or is ocean then that means that we should have
      // already joined and thus should not be here.
      CHECK( !m_[p].river.has_value() );
      CHECK( m_[p].surface != e_surface::water );
      if( !allowed_land_for_river( p, m_[p] ) ) break;
    }
    // We've not joined; start over.
    continue;
  joined:
    // This is effectively 4 in the OG.
    if( ssize( river ) < params.min_length ) continue;
    // The config validator should ensure that min_length is at
    // least 2, thus river should not be empty.
    CHECK( !river.empty() );
    // No rivers on arctic rows.
    if( on_arctic( river.back() ) ) continue;
    ++rivers;
    // Apply the river, all "minor" at this point, though don't
    // override any that may already be major (which could poten-
    // tially be at the joining point).
    for( point const p : river )
      m_[p].river = m_[p].river.value_or( e_river::minor );
    // Convert some tiles to major.
    double const major_strength =
        ( og_climate() * 2 + 12 ) * params.major_strength.value;
    if( uniform_from_1_to( major_strength ) > 6 ) {
      double const max_len_major = ( og_climate() * 2 + 3 ) *
                                   scale_linear() *
                                   params.major_length.value;
      int const len_major = uniform_from_1_to( max_len_major );
      auto const trail    = rv::reverse | rv::take( len_major );
      for( point const p : trail( river ) )
        m_[p].river = e_river::major;
    }
    CHECK( m_.exists( river.front() ) );
    on_surrounding_20( m_, river.front(),
                       [&]( point const, MapSquare& square ) {
                         if( square.surface == e_surface::water )
                           return;
                         if( square.overlay.has_value() ) return;
                         if( !coin() ) return;
                         square.overlay = e_land_overlay::forest;
                       } );
  }
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
