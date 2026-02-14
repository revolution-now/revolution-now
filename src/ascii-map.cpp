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

// base
#include "base/ansi.hpp"
#include "base/env.hpp"

// C++ standard library
#include <ostream>

namespace rn {

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;

string_view constexpr kGrasslandFg = "\033[38;5;34m";
string_view constexpr kGrasslandBg = "\033[48;5;34m";

string_view constexpr kSavannahFg = "\033[38;5;49m";
string_view constexpr kSavannahBg = "\033[48;5;49m";

string_view constexpr kArcticFg = "\033[38;5;255m";
string_view constexpr kArcticBg = "\033[48;5;255m";

string_view constexpr kDesertFg = "\033[38;5;221m";
string_view constexpr kDesertBg = "\033[48;5;221m";

string_view constexpr kPlainsFg = "\033[38;5;208m";
string_view constexpr kPlainsBg = "\033[48;5;208m";

string_view constexpr kTundraFg = "\033[38;5;230m";
string_view constexpr kTundraBg = "\033[48;5;230m";

string_view constexpr kPrairieFg = "\033[38;5;226m";
string_view constexpr kPrairieBg = "\033[48;5;226m";

// string_view constexpr kOceanFg = "\033[38;5;4m";
// string_view constexpr kOceanBg = "\033[48;5;4m";
string_view constexpr kOceanFg = base::ansi::black;
string_view constexpr kOceanBg = base::ansi::on_black;

string_view fg_color_for( MapSquare const& square ) {
  if( square.surface == e_surface::water ) return kOceanFg;
  switch( square.ground ) {
    using enum e_ground_terrain;
    case arctic:
      return kArcticFg;
    case desert:
      return kDesertFg;
    case grassland:
      return kGrasslandFg;
    case marsh:
      return base::ansi::cyan;
    case plains:
      return kPlainsFg;
    case prairie:
      return kPrairieFg;
    case savannah:
      return kSavannahFg;
    case swamp:
      return base::ansi::magenta;
    case tundra:
      return kTundraFg;
  }
}

string_view bg_color_for( MapSquare const& square ) {
  if( square.surface == e_surface::water ) return kOceanBg;
  switch( square.ground ) {
    using enum e_ground_terrain;
    case arctic:
      return kArcticBg;
    case desert:
      return kDesertBg;
    case grassland:
      return kGrasslandBg;
    case marsh:
      return base::ansi::on_cyan;
    case plains:
      return kPlainsBg;
    case prairie:
      return kPrairieBg;
    case savannah:
      return kSavannahBg;
    case swamp:
      return base::ansi::on_magenta;
    case tundra:
      return kTundraBg;
  }
}

tuple<string_view, maybe<string_view>, maybe<string_view>>
colors_for( MapSquare const& above, MapSquare const& below ) {
  if( above.surface == e_surface::water &&
      below.surface == e_surface::water )
    return { " ", fg_color_for( above ), bg_color_for( below ) };
  if( above.surface == e_surface::land &&
      below.surface == e_surface::land )
    return { "▀", fg_color_for( above ), bg_color_for( below ) };
  if( above.surface == e_surface::land &&
      below.surface == e_surface::water )
    return { "▀", fg_color_for( above ), bg_color_for( below ) };
  if( above.surface == e_surface::water &&
      below.surface == e_surface::land )
    return { "▄", fg_color_for( below ), bg_color_for( above ) };
  SHOULD_NOT_BE_HERE;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
void print_ascii_map( RealTerrain const& terrain,
                      ostream& out ) {
  auto const& m      = terrain.map;
  int const columns  = base::os_terminal_columns().value_or( 0 );
  int const n_indent = std::max( columns - m.size().w, 0 ) / 2;
  string const indent = string( n_indent, ' ' );
  auto const bar      = [&] {
    out << indent << format( "+" );
    for( X x = 0; x < 0 + m.size().w; ++x ) out << format( "-" );
    out << format( "+\n" );
  };
  bar();
  CHECK( m.size().h % 2 == 0,
         "The ascii printer requires an even map height." );
  for( Y y = 0; y < 0 + m.size().h; y += 2 ) {
    out << indent << format( "|" );
    for( X x = 0; x < 0 + m.size().w; ++x ) {
      auto const [block, color_fg, color_bg] =
          colors_for( m[y][x], m[y + 1][x] );
      out << format( "{}{}{}{}", color_fg.value_or( "" ),
                     color_bg.value_or( "" ), block,
                     base::ansi::reset );
      if( block != " " ) continue;

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
    }
    out << format( "|\n" );
  }
  bar();
}

} // namespace rn
