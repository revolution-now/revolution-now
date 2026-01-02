/****************************************************************
**map-gen.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Game map generator.
*
*****************************************************************/
#include "map-gen.hpp"

// Revolution Now
#include "connectivity.hpp"
#include "engine.hpp"
#include "game-setup.hpp"
#include "irand.hpp"
#include "lua.hpp"
#include "map-square.hpp"
#include "map-updater.hpp"
#include "terrain-mgr.hpp"

// config
#include "config/map-gen.rds.hpp"

// ss
#include "ss/land-view.hpp"
#include "ss/map-square.hpp"
#include "ss/ref.hpp"
#include "ss/root.hpp" // FIXME
#include "ss/terrain.hpp"

// luapp
#include "luapp/ext-refl.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// gfx
#include "gfx/iter.hpp"

// rand
#include "rand/entropy.hpp"
#include "rand/perlin-hashes.hpp"
#include "rand/perlin.hpp"

// base
#include "base/logger.hpp"
#include "base/timer.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::ScopedTimer;
using ::gfx::dpoint;
using ::gfx::dsize;
using ::gfx::Matrix;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::rect_iterator;
using ::gfx::size;

// We cannot allow the player to build colonies on islands be-
// cause that would give the player a way to have a coastal
// colony that could not be attacked by the royal forces during
// the war of independence because they would have no adjacent
// land squares on which to land. This would basically cause the
// game to be unwinnable.
//
// Thus, the default game rules will not allow building a colony
// on an island, which includes an arctic island.
//
// However, it still is a good idea to remove islands from the
// map even though they wouldn't cause any problems. This is be-
// cause they add noise to the map and are useless to the player.
// Also, the OG removes islands.
void remove_islands( RealTerrain& real_terrain ) {
  Matrix<MapSquare>& m = real_terrain.map;
  for( point const p : rect_iterator( m.rect() ) )
    if( is_island( real_terrain, p ) )
      m[p].surface = e_surface::water;
}

// Outside of the land zone there will be no land tiles allowed
// with the exception of arctic tiles which will be added later.
//
// NOTE: generally these buffers do not need to be scaled with
// map size because exclusion around the map edges is done by the
// suppression mechanism; these buffers are just the minimum size
// in order to enforce a couple basic rules such as:
//   1. No land touching the left/right edge of the map.
//   2. The right edge should have at least a few squares of sea
//      lane so that the ship's starting position can't see land.
// Thus, we don't scale up the buffers when the map increases in
// size beyond the standard size. That said, we do scale them
// down for smaller map sizes, since e.g. for a 16x16 map a
// buffer of 3 on the right side is a bit too big.
[[nodiscard]] rect compute_land_zone( size const world_sz ) {
  auto const round_buffer = []( double const target ) {
    return std::min( std::max( lround( target ), 1L ), 10L );
  };

  int const h_top    = 1;
  int const h_bottom = 1;
  // See note above function for the purpose of these min's.
  int const w_left =
      round_buffer( 2.0 * std::min( world_sz.w / 56.0, 1.0 ) );
  int const w_right =
      round_buffer( 3.0 * std::min( world_sz.w / 56.0, 1.0 ) );

  return rect{ .size = world_sz }
      .with_new_top_edge( h_top )
      .with_new_bottom_edge( world_sz.h - h_bottom )
      .with_new_left_edge( w_left )
      .with_new_right_edge( world_sz.w - w_right );
}

[[maybe_unused]] void suppress_edges( point const p,
                                      size const sz,
                                      double& level ) {
  int const dist_x_unscaled =
      ( p.x < sz.w / 2 ) ? p.x : sz.w - p.x - 1;
  int const dist_y_unscaled =
      ( p.y < sz.h / 2 ) ? p.y : sz.h - p.y - 1;

  dsize const scale{
    .w = sqrt( sz.w / 56.0 ),
    .h = sqrt( sz.h / 70.0 ),
  };

  // x2 for y for less suppression near top/bottom edges.
  int const dist_x = lround( dist_x_unscaled / scale.w );
  int const dist_y = lround( dist_y_unscaled / scale.h ) * 2;

  int const dist = std::min( dist_x, dist_y );

  double const kDecay =
      config_map_gen.perlin_map.suppression_decay;

  double sub = 1.0;
  for( int i = 0; i < dist; ++i ) sub *= kDecay;
  level -= sub;

  level = std::clamp( level, 0.0, 1.0 );
}

[[maybe_unused]] void generate_terrain_lua( lua::state& st ) {
  CHECK_HAS_VALUE( st["map_gen"]["generate"].pcall() );
}

[[maybe_unused]] void generate_terrain_perlin(
    lua::state&, Matrix<MapSquare>& m, rng::seed const& seed ) {
  auto const sz = config_map_gen.perlin_map.size;
  // double const kSeaLevel =
  // config_map_gen.perlin_map.sea_level;
  double const kScale = config_map_gen.perlin_map.scale;
  rng::PerlinFractalOptions const fractal_options{
    .n_octaves   = config_map_gen.perlin_map.n_octaves,
    .persistence = config_map_gen.perlin_map.persistence,
    .lacunarity  = config_map_gen.perlin_map.lacunarity,
  };
  rng::vec2 const kNoRepeat{ .x = 100'000'000,
                             .y = 100'000'000 };
  // Repeat behavior of parameters:
  //   offset: repeats every rng::kNumUniquePerlinHashes*kScale.
  //   base:   repeats every rng::kNumUniquePerlinHashes.
  uint32_t const offset_x = seed.e1;
  uint32_t const offset_y = seed.e2;
  uint32_t const base     = seed.e3 & rng::kPerlinHashMask;
  auto const perlin_noise = [&]( point const p_real ) {
    rng::vec2 const p{ .x = p_real.x * 1.0 + offset_x,
                       .y = p_real.y * 1.0 + offset_y };
    double const noise = rng::perlin_noise_2d(
        p / kScale, fractal_options, kNoRepeat, base );
    // Normalize from range [-1,1] to [0,1].
    return ( noise + 1.0 ) / 2.0;
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

  if( config_map_gen.perlin_map.suppress_edges ) {
    ScopedTimer const timer( "edge suppression" );
    for( point const p : rect_iterator( pm.rect() ) )
      suppress_edges( p, sz, pm[p] );
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
  auto const sea_level_is = [&]( double const sea_level ) {
    double const density = land_density( sea_level );
    // lg.info( "trying sea_level={} --> density={}",
    //               sea_level, density );
    double const target =
        config_map_gen.perlin_map.target_density;
    if( abs( density - target ) < kTolerance )
      return e_sea_level::good;
    if( density < target ) return e_sea_level::too_high;
    return e_sea_level::too_low;
  };
  double sea_level_min = 0.0;
  double sea_level_max = 1.0;
  [&] {
    ScopedTimer const timer( "sea level search" );
    using enum e_sea_level;
    for( int i = 0; i < 100; ++i ) {
      double const sea_level =
          ( sea_level_min + sea_level_max ) / 2.0;
      switch( sea_level_is( sea_level ) ) {
        case good:
          lg.info( "bisections: {}", i + 1 );
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
  // CHECK( sea_level_is( sea_level ) == e_sea_level::good );

  print_stats( "4" );

#if 1
  {
    ScopedTimer const timer( "land zone exclusion" );
    rect const land_zone = compute_land_zone( pm.size() );
    lg.info(
        "buffer.left={}, buffer.right={}, buffer.top={}, "
        "buffer.bottom={}",
        land_zone.left(), sz.w - land_zone.right(),
        land_zone.top(), sz.h - land_zone.bottom() );
    for( point const p : rect_iterator( pm.rect() ) )
      if( !p.is_inside( land_zone ) ) //
        pm[p] = 0.0;                  // force water.
  }
#endif

  print_stats( "5" );

  lg.info( "land density: {:.3}", land_density( sea_level ) );

  m = Matrix<MapSquare>( sz );
  {
    ScopedTimer const timer( "final map build" );
    for( point const p : rect_iterator( m.rect() ) )
      m[p].surface = ( pm[p] <= sea_level ) ? e_surface::water
                                            : e_surface::land;
  }
}

void generate_terrain( GeneratedTerrainSetup const& setup,
                       lua::state& st, IMapUpdater& map_updater,
                       rng::seed const& seed ) {
  ScopedTimer const timer( "total map generation time" );
  map_updater.modify_entire_map_no_redraw(
      [&]( RealTerrain& real_terrain ) {
        if constexpr( true ) {
          {
            ScopedTimer const timer( "generate_terrain_perlin" );
            generate_terrain_perlin( st, real_terrain.map,
                                     seed );
          }
          {
            ScopedTimer const timer( "remove_Xs" );
            CHECK_HAS_VALUE( st["map_gen"]["remove_Xs"].pcall(
                setup.land_generator.remove_Xs_probability ) );
          }
          {
            ScopedTimer const timer( "remove_islands" );
            remove_islands( real_terrain );
          }
        } else {
          (void)seed;
          (void)real_terrain;
          ScopedTimer const timer( "generate_terrain_lua" );
          generate_terrain_lua( st );
        }
      } );
}

} // namespace

void reset_terrain( IMapUpdater& map_updater, Delta size ) {
  map_updater.modify_entire_map_no_redraw(
      [&]( RealTerrain& real_terrain ) {
        real_terrain.map = gfx::Matrix<MapSquare>( size );
      } );
}

void ascii_map_gen( IEngine& engine ) {
  lua::state st;
  lua_init( st );
  SS ss;
  st["ROOT"] = ss.root;
  st["SS"]   = ss;
  TerrainConnectivity connectivity;
  NonRenderingMapUpdater non_rendering_map_updater(
      ss, connectivity );
  st["IMapUpdater"] =
      static_cast<IMapUpdater&>( non_rendering_map_updater );
  IRand& rand = engine.rand();
  st["IRand"] = static_cast<IRand&>( rand );

  rng::seed const seed =
      config_map_gen.perlin_map.seed.has_value()
          ? *config_map_gen.perlin_map.seed
          : rand.generate_deterministic_seed();
  lg.info( "seed: {}", seed );

  // ------------------------------------------------------------
  // GameSetup
  // ------------------------------------------------------------
  GameSetup setup;
  auto& generated_map_setup =
      setup.map.source.emplace<MapSource::generate>();
  generated_map_setup.terrain.land_generator
      .remove_Xs_probability =
      config_map_gen.land_generation.remove_x_probability
          .fraction;

  // ------------------------------------------------------------
  // Generate map.
  // ------------------------------------------------------------
  NonRenderingMapUpdater map_updater( ss, connectivity );
  generate_terrain( generated_map_setup.terrain, st, map_updater,
                    seed );
  gfx::Matrix<MapSquare> const& world_map =
      ss.terrain.world_map();

  // ------------------------------------------------------------
  // Print map.
  // ------------------------------------------------------------
  auto bar = [&] {
    fmt::print( "+" );
    for( X x = 0; x < 0 + world_map.size().w; ++x )
      fmt::print( "-" );
    fmt::print( "+\n" );
  };
  bar();
  for( Y y = 0; y < 0 + world_map.size().h; y += 2 ) {
    fmt::print( "|" );
    for( X x = 0; x < 0 + world_map.size().w; ++x ) {
      bool land_top =
          ( world_map[y][x].surface == e_surface::land );
      bool land_bottom =
          ( world_map[y + 1][x].surface == e_surface::land );
      if( land_top || land_bottom ) {
        int mask = ( ( land_top ? 1 : 0 ) << 1 ) |
                   ( land_bottom ? 1 : 0 );
        string c = " ";
        switch( mask ) {
          case 0b01:
            c = "▄";
            break;
          case 0b10:
            c = "▀";
            break;
          case 0b11:
            c = "█";
            break;
          default:
            SHOULD_NOT_BE_HERE;
        }
        fmt::print( "{}", c );
        continue;
      }

      bool sea_lane_top     = world_map[y][x].sea_lane;
      bool seal_lane_bottom = world_map[y + 1][x].sea_lane;
      if( sea_lane_top || seal_lane_bottom ) {
        int mask = ( ( sea_lane_top ? 1 : 0 ) << 1 ) |
                   ( seal_lane_bottom ? 1 : 0 );
        string c = " ";
        switch( mask ) {
          case 0b01:
            c = "╦";
            break;
          case 0b10:
            c = "╩";
            break;
          case 0b11:
            c = "╬";
            break;
          default:
            SHOULD_NOT_BE_HERE;
        }
        fmt::print( "{}", c );
        continue;
      }
      fmt::print( "{}", " " );
    }
    fmt::print( "|\n" );
  }
  bar();
}

void linker_dont_discard_module_map_gen();
void linker_dont_discard_module_map_gen() {}

/****************************************************************
** Lua Bindings.
*****************************************************************/
namespace {

LUA_FN( remove_islands, void ) {
  st["IMapUpdater"]
      .as<IMapUpdater&>()
      .modify_entire_map_no_redraw( remove_islands );
}

}

} // namespace rn
