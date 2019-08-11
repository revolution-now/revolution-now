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
#include "errors.hpp"
#include "gfx.hpp"
#include "init.hpp"
#include "logging.hpp"
#include "tiles.hpp"
#include "util.hpp"

using namespace std;

namespace rn {

namespace {

Square const L = Square{e_crust::land};
Square const O = Square{e_crust::water};

constexpr auto world_size = Delta{120_w, 60_h};
Matrix<Square> world_map( world_size, O );

constexpr Scale terrain_block_size{30_sx, 30_sy};

static_assert( world_size % terrain_block_size == Delta{} );

struct TerrainBlockCache {
  void redraw_if_needed() {
    if( !needs_redraw ) return;
    lg.debug( "redrawing terrain block {}", block_coord );
    Rect tiles = Rect::from( block_coord, Delta{1_w, 1_h} ) *
                 terrain_block_size;
    for( auto coord : tiles )
      render_terrain_square(
          tx, coord,
          Coord{} +
              ( coord - tiles.upper_left() ) * g_tile_scale );
    needs_redraw = false;
  }

  bool    needs_redraw{true};
  Coord   block_coord;
  Texture tx;
};

Matrix<TerrainBlockCache> block_cache( world_size /
                                       terrain_block_size );

void init_terrain() {
  auto make_squares = [&]( Coord origin ) {
    for( Y y = origin.y; y < origin.y + 10_h; ++y ) {
      for( X x = origin.x; x < origin.x + 4_w; ++x )
        world_map[y][x] = L;
      for( X x = origin.x + 6_w; x < origin.x + 10_w; ++x )
        world_map[y][x] = L;
    }
  };

  make_squares( {1_x, 1_y} );
  make_squares( {20_x, 10_y} );
  make_squares( {10_x, 30_y} );
  make_squares( {70_x, 30_y} );
  make_squares( {60_x, 10_y} );
  make_squares( {40_x, 40_y} );
  make_squares( {100_x, 25_y} );

  for( auto coord : block_cache.rect() ) {
    TerrainBlockCache cache{
        /*needs_redraw=*/true, coord,
        create_texture( Delta{1_w, 1_h} * terrain_block_size *
                        g_tile_scale )};
    block_cache[coord] = std::move( cache );
  }
}

void cleanup_terrain() { block_cache.clear(); }

} // namespace

REGISTER_INIT_ROUTINE( terrain );

void render_terrain_square( Texture& tx, Coord world_square,
                            Coord pixel_coord ) {
  auto   s = square_at( world_square );
  g_tile tile =
      s.crust == +e_crust::land ? g_tile::land : g_tile::water;
  int rot = 0;
  if( tile == g_tile::water || tile == g_tile::land )
    rot = ( world_square.x._ + world_square.y._ ) % 4;
  render_sprite( tx, tile, pixel_coord, rot, 0 );
}

void render_terrain( Rect src_tiles, Texture& dest,
                     Coord dest_pixel_coord ) {
  Rect blocks =
      Rect::from( src_tiles.upper_left() / terrain_block_size,
                  src_tiles.lower_right().rounded_up_to_multiple(
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

Delta world_size_tiles() { return world_map.size(); }

Delta world_size_pixels() {
  auto delta = world_size_tiles();
  return {delta.h * g_tile_height, delta.w * g_tile_width};
}

Rect world_rect_tiles() {
  return {0_x, 0_y, world_size_tiles().w, world_size_tiles().h};
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

Opt<Ref<Square const>> maybe_square_at( Coord coord ) {
  if( !square_exists( coord.y, coord.x ) ) return nullopt;
  return world_map[coord.y][coord.x];
}

Square const& square_at( Coord coord ) {
  auto res = maybe_square_at( coord );
  CHECK( res, "square {} does not exist!", coord );
  return *res;
}

} // namespace rn
