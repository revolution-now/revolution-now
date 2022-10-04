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
#include "imap-updater.hpp"
#include "map-square.hpp"

// ss
#include "ss/land-view.hpp"
#include "ss/map-square.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

void generate_terrain_impl( lua::state& st,
                            Matrix<MapSquare>& ) {
  CHECK_HAS_VALUE( st["map_gen"]["generate"].pcall() );
}

} // namespace

void reset_terrain( IMapUpdater& map_updater, Delta size ) {
  map_updater.modify_entire_map(
      [&]( Matrix<MapSquare>& world_map ) {
        world_map = Matrix<MapSquare>( size );
      } );
}

void generate_terrain( lua::state&  st,
                       IMapUpdater& map_updater ) {
  map_updater.modify_entire_map( [&]( Matrix<MapSquare>& m ) {
    generate_terrain_impl( st, m );
  } );
}

void ascii_map_gen( lua::state& st, SS& ss ) {
  NonRenderingMapUpdater map_updater( ss );
  generate_terrain( st, map_updater );
  Matrix<MapSquare> const& world_map = ss.terrain.world_map();

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

void linker_dont_discard_module_map_gen();
void linker_dont_discard_module_map_gen() {}

} // namespace rn
