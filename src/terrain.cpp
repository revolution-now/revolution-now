/****************************************************************
**terrain.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-27.
*
* Description: Representation of the physical world.
*
*****************************************************************/
#include "terrain.hpp"

// Revolution Now
#include "error.hpp"
#include "game-state.hpp"
#include "gfx.hpp"
#include "gs-terrain.hpp"
#include "init.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "macros.hpp"
#include "matrix.hpp"
#include "tiles.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

bool g_use_block_cache = false;
bool g_show_grid       = false;

/****************************************************************
** Save-Game State
*****************************************************************/
constexpr Scale terrain_block_size{ 50_sx, 50_sy };

static_assert( world_size % terrain_block_size == Delta{} );

struct TerrainBlockCache {
  void redraw_if_needed() {
    if( !needs_redraw ) return;
    lg.debug( "redrawing terrain block {}", block_coord );
    Rect tiles = Rect::from( block_coord, Delta{ 1_w, 1_h } ) *
                 terrain_block_size;
    for( auto coord : tiles )
      render_terrain_square(
          tx, coord,
          Coord{} +
              ( coord - tiles.upper_left() ) * g_tile_scale );
    needs_redraw = false;
  }

  bool    needs_redraw{ true };
  Coord   block_coord;
  Texture tx;
};
NOTHROW_MOVE( TerrainBlockCache );

Matrix<TerrainBlockCache> block_cache( world_size /
                                       terrain_block_size );

void invalidate_caches() {
  for( Coord coord : block_cache.rect() )
    block_cache[coord].needs_redraw = true;
}

void init_terrain() {
  lg.debug( "terrain block cache is {}.",
            g_use_block_cache ? "on" : "off" );
  for( auto coord : block_cache.rect() ) {
    TerrainBlockCache cache{
        /*needs_redraw=*/true, coord,
        create_texture( Delta{ 1_w, 1_h } * terrain_block_size *
                        g_tile_scale ) };
    block_cache[coord] = std::move( cache );
  }
}

void cleanup_terrain() { block_cache.clear(); }

REGISTER_INIT_ROUTINE( terrain );

} // namespace

void generate_terrain() {
  TerrainState&    terrain_state = GameState::terrain();
  LandSquare const L             = LandSquare{ e_surface::land };
  LandSquare const O = LandSquare{ e_surface::water };

  auto& world_map = terrain_state.world_map;
  // FIXME
  world_map = Matrix<LandSquare>( world_size );

  for( auto const& coord : terrain_state.world_map.rect() )
    world_map[coord] = O;

  auto make_squares = [&]( Coord origin ) {
    for( Y y = origin.y; y < origin.y + 10_h; ++y ) {
      for( X x = origin.x; x < origin.x + 4_w; ++x )
        terrain_state.world_map[y][x] = L;
      for( X x = origin.x + 6_w; x < origin.x + 10_w; ++x )
        terrain_state.world_map[y][x] = L;
    }
  };

  make_squares( { 1_x, 1_y } );
  make_squares( { 20_x, 10_y } );
  make_squares( { 10_x, 30_y } );
  make_squares( { 70_x, 30_y } );
  make_squares( { 60_x, 10_y } );
  make_squares( { 40_x, 40_y } );
  make_squares( { 100_x, 25_y } );
}

void render_terrain_square( Texture& tx, Coord world_square,
                            Coord pixel_coord ) {
  auto   s    = square_at( world_square );
  e_tile tile = s.surface == e_surface::land ? e_tile::land
                                             : e_tile::water;
  render_sprite( tx, tile, pixel_coord, 0, 0 );
  if( g_show_grid )
    render_rect(
        tx, gfx::pixel( 0, 0, 0, 30 ),
        Rect::from( pixel_coord,
                    g_tile_delta + Delta( 1_w, 1_h ) ) );
}

void render_terrain_nocache( Rect src_tiles, Texture& dest,
                             Coord dest_pixel_coord ) {
  for( auto square : src_tiles )
    render_terrain_square(
        dest, square,
        dest_pixel_coord +
            ( ( square - src_tiles.upper_left() ) *
              g_tile_scale ) );
}

void render_terrain_cache( Rect src_tiles, Texture& dest,
                           Coord dest_pixel_coord ) {
  Rect blocks = Rect::from(
      src_tiles.upper_left() / terrain_block_size,
      src_tiles.lower_right().rounded_to_multiple_to_plus_inf(
          terrain_block_size ) /
          terrain_block_size );

  Coord origin = dest_pixel_coord -
                 g_tile_scale * ( src_tiles.upper_left() %
                                  terrain_block_size );
  for( auto block : blocks ) {
    auto& cache = block_cache[block];
    cache.redraw_if_needed();
    auto dest_coord = origin + ( block - blocks.upper_left() ) *
                                   terrain_block_size *
                                   g_tile_scale;
    copy_texture( cache.tx, dest, dest_coord );
  }
}

void render_terrain( Rect src_tiles, Texture& dest,
                     Coord dest_pixel_coord ) {
  auto f = g_use_block_cache ? render_terrain_cache
                             : render_terrain_nocache;
  f( src_tiles, dest, dest_pixel_coord );
}

Delta world_size_tiles() {
  TerrainState& terrain_state = GameState::terrain();
  return terrain_state.world_map.size();
}

Delta world_size_pixels() {
  auto delta = world_size_tiles();
  return { delta.h * g_tile_height, delta.w * g_tile_width };
}

Rect world_rect_tiles() {
  return { 0_x, 0_y, world_size_tiles().w,
           world_size_tiles().h };
}

Rect world_rect_pixels() {
  return Rect::from( Coord{}, world_size_pixels() );
}

bool square_exists( Y y, X x ) {
  if( y < 0 || x < 0 ) return false;
  auto [w, h] = world_size_tiles();
  return 0_y + h > y && 0_x + w > x;
}

bool square_exists( Coord coord ) {
  return square_exists( coord.y, coord.x );
}

maybe<LandSquare const&> maybe_square_at( Coord coord ) {
  TerrainState& terrain_state = GameState::terrain();
  if( !square_exists( coord.y, coord.x ) ) return nothing;
  return terrain_state.world_map[coord.y][coord.x];
}

LandSquare const& square_at( Coord coord ) {
  auto res = maybe_square_at( coord );
  CHECK( res, "square {} does not exist!", coord );
  return *res;
}

bool terrain_is_land( Coord coord ) {
  switch( square_at( coord ).surface ) {
    case e_surface::land: return true;
    case e_surface::water: return false;
  }
}

/****************************************************************
** Testing
*****************************************************************/
void generate_unittest_terrain() {
  LandSquare const L = LandSquare{ e_surface::land };
  LandSquare const O = LandSquare{ e_surface::water };

  TerrainState& terrain_state = GameState::terrain();
  auto&         world_map     = terrain_state.world_map;
  world_map                   = Matrix<LandSquare>( 10_w, 10_h );

  Rect land_rect{ 2_x, 2_y, 6_w, 6_h };

  for( auto const& coord : terrain_state.world_map.rect() ) {
    world_map[coord] = O;
    if( coord.is_inside( land_rect ) ) world_map[coord] = L;
  }
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( toggle_surface, void, Coord const& coord ) {
  TerrainState& terrain_state = GameState::terrain();
  CHECK( coord.is_inside( terrain_state.world_map.rect() ),
         "coordinate {} is out of bounds.", coord );
  terrain_state.world_map[coord].surface =
      terrain_state.world_map[coord].surface == e_surface::land
          ? e_surface::water
          : e_surface::land;
  invalidate_caches();
}

LUA_FN( generate_terrain, void ) { generate_terrain(); }

LUA_FN( toggle_block_cache, void ) {
  g_use_block_cache = !g_use_block_cache;
  lg.debug( "terrain block cache is {}.",
            g_use_block_cache ? "on" : "off" );
}

LUA_FN( toggle_grid, void ) {
  g_show_grid = !g_show_grid;
  lg.debug( "terrain grid is {}.", g_show_grid ? "on" : "off" );
}

} // namespace

} // namespace rn
