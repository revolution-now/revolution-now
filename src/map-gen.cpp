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
#include "lua.hpp"
#include "map-square.hpp"
#include "map-updater.hpp"
#include "rand.hpp"

// ss
#include "ss/land-view.hpp"
#include "ss/map-square.hpp"
#include "ss/ref.hpp"
#include "ss/root.hpp" // FIXME
#include "ss/terrain.hpp"

// luapp
#include "luapp/ext-refl.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// gfx
#include "gfx/iter.hpp"

// rand
#include "rand/perlin-hashes.hpp"
#include "rand/perlin.hpp"
#include "rand/random.hpp"

// base
#include "base/timer.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::dpoint;
using ::gfx::dsize;
using ::gfx::Matrix;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::rect_iterator;
using ::gfx::size;

// Outside of the land zone there will be no land tiles allowed
// with the exception of arctic tiles which will be added later.
[[nodiscard]] rect compute_land_zone( size const world_sz ) {
  auto const round_buffer = []( double const target ) {
    return std::min( std::max( lround( target ), 1L ), 10L );
  };

  int const h_top    = round_buffer( world_sz.h / 70.0 );
  int const h_bottom = round_buffer( world_sz.h / 70.0 );
  int const w_left   = round_buffer( 2.0 * world_sz.w / 56.0 );
  int const w_right  = round_buffer( 3.0 * world_sz.w / 56.0 );

  return rect{ .size = world_sz }
      .with_new_top_edge( h_top )
      .with_new_bottom_edge( world_sz.h - h_bottom )
      .with_new_left_edge( w_left )
      .with_new_right_edge( world_sz.w - w_right );
}

[[maybe_unused]] void suppress_edges( point const p,
                                      size const world_sz,
                                      double const sea_level,
                                      double& level ) {
  dpoint const normalized = p.to_double() / world_sz.to_double();
  double const w_sub_decay_normalized =
      ( normalized.x < .5 ) ? normalized.x : 1.0 - normalized.x;
  double const h_sub_decay_normalized =
      ( normalized.y < .5 ) ? normalized.y : 1.0 - normalized.y;
  int const w_sub_decay =
      int( w_sub_decay_normalized / ( 1.0 / 56.0 ) );
  int const h_sub_decay =
      int( h_sub_decay_normalized / ( 1.0 / 70.0 ) ) + 2;

  double const land_section = 1.0 - sea_level;

  double sub          = 0.0;
  double const kDecay = .5;

  sub = land_section;
  for( int i = 0; i < w_sub_decay; ++i ) sub *= kDecay;
  level -= sub;

  sub = land_section;
  for( int i = 0; i < h_sub_decay; ++i ) sub *= kDecay;
  level -= sub;

  level = std::max( level, 0.0 );
}

[[maybe_unused]] void generate_terrain_lua( lua::state& st ) {
  CHECK_HAS_VALUE( st["map_gen"]["generate"].pcall() );
}

struct MapSeed {
  uint64_t const lo = {};
  uint64_t const hi = {};
};

[[maybe_unused]] void generate_terrain_perlin(
    lua::state&, gfx::Matrix<MapSquare>& m,
    MapSeed const seed ) {
  // auto const sz = size{ .w = 200, .h = 100 };
  auto const sz = size{ .w = 56, .h = 70 };
  // auto const sz = size{ .w = 16, .h = 16 };

  double const kSeaLevel = .54;
  double const kScale    = 10.0;
  rng::PerlinFractalOptions const fractal_options{
    .n_octaves = 4, .persistence = 0.5, .lacunarity = 2.0 };
  rng::vec2 const kNoRepeat{ .x = 100'000'000,
                             .y = 100'000'000 };
  // Repeat behavior of parameters:
  //   offset: repeats every rng::kNumUniquePerlinHashes*kScale.
  //   base:   repeats every rng::kNumUniquePerlinHashes.
  uint32_t const offset_x = ( seed.lo >> 0 ) & 0xffffffff;
  uint32_t const offset_y = ( seed.lo >> 32 ) & 0xffffffff;
  uint32_t const base     = seed.hi & 65535;
  auto const perlin_land  = [&]( point const p_real ) {
    rng::vec2 const p{ .x = p_real.x * 1.0 + offset_x,
                        .y = p_real.y * 1.0 + offset_y };
    double perlin_noise = rng::perlin_noise_2d(
        p / kScale, fractal_options, kNoRepeat, base );
    // Normalize from range [-1,1] to [0,1].
    perlin_noise = ( perlin_noise + 1.0 ) / 2.0;
    suppress_edges( p_real, sz, kSeaLevel, perlin_noise );
    return perlin_noise > kSeaLevel;
  };
  m                    = Matrix<MapSquare>( sz );
  rect const land_zone = compute_land_zone( m.size() );
  fmt::println( "land_zone: {}", land_zone );
  for( point const p : rect_iterator( m.rect() ) ) {
    m[p].surface = e_surface::water;
    if( !p.is_inside( land_zone ) ) continue;
    if( !perlin_land( p ) ) continue;
    m[p].surface = e_surface::land;
  }
}

void generate_terrain( lua::state& st, IMapUpdater& map_updater,
                       MapSeed const seed ) {
  map_updater.modify_entire_map_no_redraw(
      [&]( RealTerrain& real_terrain ) {
        if constexpr( true ) {
          {
            base::ScopedTimer const timer(
                "generate_terrain_perlin" );
            generate_terrain_perlin( st, real_terrain.map,
                                     seed );
          }
          {
            base::ScopedTimer const timer( "remove_islands" );
            CHECK_HAS_VALUE(
                st["map_gen"]["remove_islands"].pcall() );
          }
        } else {
          (void)seed;
          (void)real_terrain;
          base::ScopedTimer const timer(
              "generate_terrain_lua" );
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

void ascii_map_gen() {
  lua::state st;
  lua_init( st );
  SS ss;
  st["ROOT"] = ss.root;
  st["SS"]   = ss;
  TerrainConnectivity connectivity;
  Rand rand;
  st["IRand"] = static_cast<IRand&>( rand );

  rng::random rng;
  // uint32_t const seed = 3111;
  MapSeed const seed{
    .lo = rng.uniform<uint64_t>(),
    .hi = rng.uniform<uint64_t>(),
  };

  NonRenderingMapUpdater map_updater( ss, connectivity );
  generate_terrain( st, map_updater, seed );
  gfx::Matrix<MapSquare> const& world_map =
      ss.terrain.world_map();

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

} // namespace rn
