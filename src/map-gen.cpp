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
#include "rand.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

void reset_terrain( IMapUpdater& map_updater, Delta size ) {
  map_updater.modify_entire_map(
      [&]( Matrix<MapSquare>& world_map ) {
        world_map = Matrix<MapSquare>( size );
      } );
}

void generate_terrain_impl( Matrix<MapSquare>& world_map ) {
  lua::state& st = lua_global_state();
  // st["math"]["randomseed"]( rng::random_int() );
  st["map_gen"]["generate"]();

  // FIXME find a better way to do this.
  LandViewState& land_view_state = GameState::land_view();
  land_view_state.viewport.set_max_viewable_size_tiles(
      world_map.size() );
}

} // namespace

void generate_terrain( IMapUpdater& map_updater ) {
  map_updater.modify_entire_map( generate_terrain_impl );
}

void ascii_map_gen() {
  TerrainState&          terrain_state = GameState::terrain();
  NonRenderingMapUpdater map_updater( terrain_state );
  generate_terrain( map_updater );
  Matrix<MapSquare> const& world_map = terrain_state.world_map();
  auto                     bar       = [&] {
    fmt::print( "+" );
    for( X x = 0_x; x < 0_x + world_map.size().w; ++x )
      fmt::print( "-" );
    fmt::print( "+\n" );
  };
  bar();
  for( Y y = 0_y; y < 0_y + world_map.size().h; y += 2_h ) {
    fmt::print( "|" );
    for( X x = 0_x; x < 0_x + world_map.size().w; ++x ) {
      bool land_top =
          ( world_map[y][x].surface == e_surface::land );
      bool land_bottom =
          ( world_map[y + 1_h][x].surface == e_surface::land );
      if( land_top || land_bottom ) {
        int mask = ( ( land_top ? 1 : 0 ) << 1 ) |
                   ( land_bottom ? 1 : 0 );
        string c = " ";
        switch( mask ) {
          case 0b01: c = "▄"; break;
          case 0b10: c = "▀"; break;
          case 0b11: c = "█"; break;
          default: SHOULD_NOT_BE_HERE;
        }
        fmt::print( "{}", c );
        continue;
      }

      bool sea_lane_top     = world_map[y][x].sea_lane;
      bool seal_lane_bottom = world_map[y + 1_h][x].sea_lane;
      if( sea_lane_top || seal_lane_bottom ) {
        int mask = ( ( sea_lane_top ? 1 : 0 ) << 1 ) |
                   ( seal_lane_bottom ? 1 : 0 );
        string c = " ";
        switch( mask ) {
          case 0b01: c = "╦"; break;
          case 0b10: c = "╩"; break;
          case 0b11: c = "╬"; break;
          default: SHOULD_NOT_BE_HERE;
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

void linker_dont_discard_module_map_gen() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( generate_terrain, void ) {
  // FIXME: this should render, but it breaks unit tests.
  NonRenderingMapUpdater map_updater( GameState::terrain() );
  generate_terrain( map_updater );
}

LUA_FN( reset_terrain, void, Delta size ) {
  // FIXME: this should render, but it breaks unit tests.
  NonRenderingMapUpdater map_updater( GameState::terrain() );
  reset_terrain( map_updater, size );
}

LUA_FN( at, MapSquare&, Coord tile ) {
  TerrainState& terrain_state = GameState::terrain();
  // FIXME: this generates a formatting error because `tile` for-
  // mats into a string with curly braces which somehow need to
  // be escaped.
  LUA_CHECK( st, terrain_state.square_exists( tile ),
             "There is no tile at coordinate {}.", tile );
  return terrain_state.mutable_square_at( tile );
}

LUA_FN( terrain_state, TerrainState& ) {
  return GameState::terrain();
}

LUA_FN( world_size, lua::table ) {
  TerrainState& terrain_state = GameState::terrain();
  lua::table    res           = st.table.create();
  res["w"] = terrain_state.world_map().size().w;
  res["h"] = terrain_state.world_map().size().h;
  return res;
}

} // namespace

} // namespace rn
