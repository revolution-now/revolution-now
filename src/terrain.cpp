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
#include "gs-terrain.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "macros.hpp"
#include "matrix.hpp"
#include "tiles.hpp"

// render
#include "render/renderer.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

bool g_show_grid = false;

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

// Pass in the painter as well for efficiency.
void render_terrain_square( rr::Painter& painter, Rect where,
                            Coord world_square ) {
  e_tile tile =
      square_at( world_square ).surface == e_surface::land
          ? e_tile::land
          : e_tile::water;
  render_sprite( painter, where, tile );
  if( g_show_grid )
    painter.draw_empty_rect( where,
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel{ 0, 0, 0, 30 } );
}

void render_terrain_square( rr::Painter& painter, Coord where,
                            Coord world_square ) {
  render_terrain_square(
      painter, Rect::from( where, g_tile_delta ), world_square );
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
}

LUA_FN( generate_terrain, void ) { generate_terrain(); }

LUA_FN( toggle_grid, void ) {
  g_show_grid = !g_show_grid;
  lg.debug( "terrain grid is {}.", g_show_grid ? "on" : "off" );
}

} // namespace

} // namespace rn
