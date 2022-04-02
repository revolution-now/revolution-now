/****************************************************************
**world-map.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-23.
*
* Description: Handles interaction with the world map.
*
*****************************************************************/
#include "world-map.hpp"

// Revolution Now
#include "game-state.hpp"
#include "gs-land-view.hpp"
#include "gs-terrain.hpp"
#include "lua.hpp"
#include "tiles.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

MapSquare make_land_square() {
  return map_square_for_terrain( e_terrain::grassland );
}

MapSquare make_ocean_square() {
  return map_square_for_terrain( e_terrain::ocean );
}

} // namespace

void generate_terrain() {
  TerrainState&   terrain_state = GameState::terrain();
  MapSquare const L             = make_land_square();
  MapSquare const O             = make_ocean_square();

  auto& world_map = terrain_state.world_map;
  // FIXME
  world_map = Matrix<MapSquare>( world_size );

  for( auto const& coord : terrain_state.world_map.rect() )
    world_map[coord] = O;

  for( Y y = 0_y;
       y < terrain_state.world_map.rect().bottom_edge(); ++y )
    world_map[Coord( y, 0_x )].sea_lane = true;

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

  // FIXME find a better way to do this.
  LandViewState& land_view_state = GameState::land_view();
  land_view_state.viewport.set_max_viewable_size_tiles(
      world_size );
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

bool square_exists( TerrainState const& terrain_state,
                    Coord               coord ) {
  if( coord.x < 0_x || coord.y < 0_y ) return false;
  return coord.is_inside(
      Rect::from( Coord{}, terrain_state.world_map.size() ) );
}

MapSquare const& square_at( TerrainState const& terrain_state,
                            Coord               coord ) {
  auto res = maybe_square_at( terrain_state, coord );
  CHECK( res, "square {} does not exist!", coord );
  return *res;
}

MapSquare& square_at( TerrainState& terrain_state,
                      Coord         coord ) {
  auto res = maybe_square_at( terrain_state, coord );
  CHECK( res, "square {} does not exist!", coord );
  return *res;
}

MapSquare const& square_at( Coord coord ) {
  TerrainState const& terrain_state = GameState::terrain();
  return square_at( terrain_state, coord );
}

MapSquare& mutable_square_at( Coord coord ) {
  TerrainState& terrain_state = GameState::terrain();
  return square_at( terrain_state, coord );
}

maybe<MapSquare&> maybe_square_at( TerrainState& terrain_state,
                                   Coord         coord ) {
  if( !square_exists( terrain_state, coord ) ) return nothing;
  return terrain_state.world_map[coord.y][coord.x];
}

maybe<MapSquare const&> maybe_square_at(
    TerrainState const& terrain_state, Coord coord ) {
  if( !square_exists( terrain_state, coord ) ) return nothing;
  return terrain_state.world_map[coord.y][coord.x];
}

bool is_land( Coord coord ) {
  return is_land( square_at( coord ) );
}

bool is_land( TerrainState const& terrain_state, Coord coord ) {
  return is_land( square_at( terrain_state, coord ) );
}

/****************************************************************
** Testing
*****************************************************************/
// FIXME: remove
void generate_unittest_terrain() {
  MapSquare const L = make_land_square();
  MapSquare const O = make_ocean_square();

  TerrainState& terrain_state = GameState::terrain();
  auto&         world_map     = terrain_state.world_map;
  world_map                   = Matrix<MapSquare>( 10_w, 10_h );

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

LUA_FN( toggle_surface, void, Coord coord ) {
  MapSquare const L             = make_land_square();
  MapSquare const O             = make_ocean_square();
  TerrainState&   terrain_state = GameState::terrain();
  CHECK( coord.is_inside( terrain_state.world_map.rect() ),
         "coordinate {} is out of bounds.", coord );
  terrain_state.world_map[coord] = is_land( coord ) ? O : L;
}

LUA_FN( generate_terrain, void ) { generate_terrain(); }

LUA_FN( at, MapSquare&, Coord tile ) {
  TerrainState& terrain_state = GameState::terrain();
  LUA_CHECK( st, square_exists( terrain_state, tile ),
             "There is no tile at coordinate {}.", tile );
  return square_at( terrain_state, tile );
}

} // namespace

} // namespace rn
