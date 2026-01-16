/****************************************************************
**ascii-map.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-04.
*
* Description: Draws maps to the terminal for tools/testing.
*
*****************************************************************/
#include "ascii-map.hpp"

// ss
#include "ss/terrain.rds.hpp"

// C++ standard library
#include <ostream>

namespace rn {

namespace {

using namespace std;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void print_ascii_map( RealTerrain const& terrain,
                      ostream& out ) {
  auto const& m  = terrain.map;
  auto const bar = [&] {
    out << format( "+" );
    for( X x = 0; x < 0 + m.size().w; ++x ) out << format( "-" );
    out << format( "+\n" );
  };
  bar();
  CHECK( m.size().h % 2 == 0,
         "The ascii printer requires an even map height." );
  for( Y y = 0; y < 0 + m.size().h; y += 2 ) {
    out << format( "|" );
    for( X x = 0; x < 0 + m.size().w; ++x ) {
      bool land_top = ( m[y][x].surface == e_surface::land );
      bool land_bottom =
          ( m[y + 1][x].surface == e_surface::land );
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
        out << format( "{}", c );
        continue;
      }

      bool sea_lane_top     = m[y][x].sea_lane;
      bool seal_lane_bottom = m[y + 1][x].sea_lane;
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
        out << format( "{}", c );
        continue;
      }
      out << format( "{}", " " );
    }
    out << format( "|\n" );
  }
  bar();
}

} // namespace rn
