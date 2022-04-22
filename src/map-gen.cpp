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
#include "gs-land-view.hpp"
#include "gs-terrain.hpp"
#include "lua.hpp"
#include "map-square.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

inline constexpr auto world_size = Delta{ 200_w, 200_h };

MapSquare make_land_square() {
  return map_square_for_terrain( e_terrain::grassland );
}

MapSquare make_ocean_square() {
  return map_square_for_terrain( e_terrain::ocean );
}

void generate_terrain_impl( Matrix<MapSquare>& world_map ) {
  MapSquare const L = make_land_square();
  MapSquare const O = make_ocean_square();

  // FIXME
  world_map = Matrix<MapSquare>( world_size );

  for( auto const& coord : world_map.rect() )
    world_map[coord] = O;

  for( Y y = 0_y; y < world_map.rect().bottom_edge(); ++y )
    world_map[Coord( y, 0_x )].sea_lane = true;

  auto make_squares = [&]( Coord origin ) {
    for( Y y = origin.y; y < origin.y + 10_h; ++y ) {
      for( X x = origin.x; x < origin.x + 4_w; ++x )
        world_map[y][x] = L;
      for( X x = origin.x + 6_w; x < origin.x + 10_w; ++x )
        world_map[y][x] = L;
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

} // namespace

void generate_terrain( IMapUpdater const& map_updater ) {
  map_updater.modify_entire_map( generate_terrain_impl );
}

void linker_dont_discard_module_map_gen() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( generate_terrain, void ) {
  generate_terrain(
      // FIXME: this should render, but it breaks unit tests.
      NonRenderingMapUpdater( GameState::terrain() ) );
}

LUA_FN( at, MapSquare&, Coord tile ) {
  TerrainState& terrain_state = GameState::terrain();
  LUA_CHECK( st, terrain_state.square_exists( tile ),
             "There is no tile at coordinate {}.", tile );
  return terrain_state.mutable_square_at( tile );
}

} // namespace

} // namespace rn
