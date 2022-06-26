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
#include "lua.hpp"
#include "map-square.hpp"

// gs
#include "gs/land-view.hpp"
#include "gs/map-square.hpp"
#include "gs/terrain.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

void generate_terrain_impl( Matrix<MapSquare>& ) {
  lua::state& st = lua_global_state();
  CHECK_HAS_VALUE( st["map_gen"]["generate"].pcall() );
}

} // namespace

void reset_terrain( IMapUpdater& map_updater, Delta size ) {
  map_updater.modify_entire_map(
      [&]( Matrix<MapSquare>& world_map ) {
        world_map = Matrix<MapSquare>( size );
      } );
}

void generate_terrain( IMapUpdater& map_updater ) {
  map_updater.modify_entire_map( generate_terrain_impl );
}

void ascii_map_gen() {
  TerrainState&          terrain_state = GameState::terrain();
  NonRenderingMapUpdater map_updater( terrain_state );
  generate_terrain( map_updater );
  Matrix<MapSquare> const& world_map = terrain_state.world_map();

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
          case 0b01: c = "▄"; break;
          case 0b10: c = "▀"; break;
          case 0b11: c = "█"; break;
          default: SHOULD_NOT_BE_HERE;
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

LUA_FN( reset_terrain, void, Delta size ) {
  NonRenderingMapUpdater map_updater( GameState::terrain() );
  reset_terrain( map_updater, size );
}

// FIXME: get rid of this and access it via the TerrainState
// which is available from Lua.
LUA_FN( at, MapSquare&, Coord tile ) {
  TerrainState& terrain_state = GameState::terrain();
  maybe<e_cardinal_direction> d =
      terrain_state.proto_square_direction_for_tile( tile );
  if( d.has_value() )
    return terrain_state.mutable_proto_square( *d );
  // This should never fail since coord should now be on the map.
  return terrain_state.mutable_square_at( tile );
}

// FIXME: get rid of this; lua should access the terrain state
// via the root state.
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
