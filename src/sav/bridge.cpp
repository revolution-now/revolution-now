/****************************************************************
**bridge.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-12.
*
* Description: TODO [FILL ME IN]
*
*****************************************************************/
#include "bridge.hpp"

// sav
#include "map-file.hpp"
#include "sav-struct.hpp"

// ss
#include "ss/root.hpp"
#include "terrain-enums.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/timer.hpp"

// C++ standard library
#include <type_traits>

using namespace std;

namespace bridge {

namespace {

using ::base::expect;
using ::base::nothing;
using ::base::ScopedTimer;
using ::base::valid;
using ::base::valid_or;

// TODO: replace this with std::to_underlying when C++23 becomes
// available.
template<typename E>
requires is_enum_v<E>
auto to_underlying( E e ) {
  return static_cast<underlying_type_t<E>>( e );
}

expect<rn::MapSquare, string> map_square_from_tile(
    sav::TILE tile ) {
  rn::MapSquare res;
  switch( tile.tile ) {
    case sav::terrain_5bit_type::tu:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::tundra;
      break;
    case sav::terrain_5bit_type::de:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::desert;
      break;
    case sav::terrain_5bit_type::pl:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::plains;
      break;
    case sav::terrain_5bit_type::pr:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::prairie;
      break;
    case sav::terrain_5bit_type::gr:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::grassland;
      break;
    case sav::terrain_5bit_type::sa:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::savannah;
      break;
    case sav::terrain_5bit_type::sw:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::swamp;
      break;
    case sav::terrain_5bit_type::mr:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::marsh;
      break;
    case sav::terrain_5bit_type::tuf:
    case sav::terrain_5bit_type::tuw:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::tundra;
      res.overlay = rn::e_land_overlay::forest;
      break;
    case sav::terrain_5bit_type::def:
    case sav::terrain_5bit_type::dew:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::desert;
      res.overlay = rn::e_land_overlay::forest;
      break;
    case sav::terrain_5bit_type::plf:
    case sav::terrain_5bit_type::plw:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::plains;
      res.overlay = rn::e_land_overlay::forest;
      break;
    case sav::terrain_5bit_type::prf:
    case sav::terrain_5bit_type::prw:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::prairie;
      res.overlay = rn::e_land_overlay::forest;
      break;
    case sav::terrain_5bit_type::grf:
    case sav::terrain_5bit_type::grw:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::grassland;
      res.overlay = rn::e_land_overlay::forest;
      break;
    case sav::terrain_5bit_type::saf:
    case sav::terrain_5bit_type::saw:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::savannah;
      res.overlay = rn::e_land_overlay::forest;
      break;
    case sav::terrain_5bit_type::swf:
    case sav::terrain_5bit_type::sww:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::swamp;
      res.overlay = rn::e_land_overlay::forest;
      break;
    case sav::terrain_5bit_type::mrf:
    case sav::terrain_5bit_type::mrw:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::marsh;
      res.overlay = rn::e_land_overlay::forest;
      break;
    case sav::terrain_5bit_type::arc:
      res.surface = rn::e_surface::land;
      res.ground  = rn::e_ground_terrain::arctic;
      break;
    case sav::terrain_5bit_type::ttt:
      res.surface = rn::e_surface::water;
      break;
    case sav::terrain_5bit_type::tnt:
      res.surface  = rn::e_surface::water;
      res.sea_lane = true;
      break;
  }
  switch( tile.hill_river ) {
    case sav::hills_river_3bit_type::empty:
      break;
    case sav::hills_river_3bit_type::c:
      res.overlay = rn::e_land_overlay::hills;
      break;
    case sav::hills_river_3bit_type::t:
      res.river = rn::e_river::minor;
      break;
    case sav::hills_river_3bit_type::tc:
      res.overlay = rn::e_land_overlay::hills;
      res.river   = rn::e_river::minor;
      break;
    case sav::hills_river_3bit_type::cc:
      res.overlay = rn::e_land_overlay::mountains;
      break;
    case sav::hills_river_3bit_type::tt:
      res.river = rn::e_river::major;
      break;
    default:
      return fmt::format(
          "unsupported value for tile.hill_river: {:03b}",
          to_underlying( tile.hill_river ) );
  }
  return res;
}

expect<sav::TILE, string> tile_from_map_square(
    rn::MapSquare const& square ) {
  sav::TILE res = {};
  if( square.surface == rn::e_surface::water ) {
    res.tile = square.sea_lane ? sav::terrain_5bit_type::tnt
                               : sav::terrain_5bit_type::ttt;
    return res;
  }
  switch( square.ground ) {
    case rn::e_ground_terrain::arctic:
      res.tile = sav::terrain_5bit_type::arc;
      break;
    case rn::e_ground_terrain::desert:
      res.tile = ( square.overlay == rn::e_land_overlay::forest )
                     ? sav::terrain_5bit_type::def
                     : sav::terrain_5bit_type::de;
      break;
    case rn::e_ground_terrain::grassland:
      res.tile = ( square.overlay == rn::e_land_overlay::forest )
                     ? sav::terrain_5bit_type::grf
                     : sav::terrain_5bit_type::gr;
      break;
    case rn::e_ground_terrain::marsh:
      res.tile = ( square.overlay == rn::e_land_overlay::forest )
                     ? sav::terrain_5bit_type::mrf
                     : sav::terrain_5bit_type::mr;
      break;
    case rn::e_ground_terrain::plains:
      res.tile = ( square.overlay == rn::e_land_overlay::forest )
                     ? sav::terrain_5bit_type::plf
                     : sav::terrain_5bit_type::pl;
      break;
    case rn::e_ground_terrain::prairie:
      res.tile = ( square.overlay == rn::e_land_overlay::forest )
                     ? sav::terrain_5bit_type::prf
                     : sav::terrain_5bit_type::pr;
      break;
    case rn::e_ground_terrain::savannah:
      res.tile = ( square.overlay == rn::e_land_overlay::forest )
                     ? sav::terrain_5bit_type::saf
                     : sav::terrain_5bit_type::sa;
      break;
    case rn::e_ground_terrain::swamp:
      res.tile = ( square.overlay == rn::e_land_overlay::forest )
                     ? sav::terrain_5bit_type::swf
                     : sav::terrain_5bit_type::sw;
      break;
    case rn::e_ground_terrain::tundra:
      res.tile = ( square.overlay == rn::e_land_overlay::forest )
                     ? sav::terrain_5bit_type::tuf
                     : sav::terrain_5bit_type::tu;
      break;
  }
  bool const has_hills =
      ( square.overlay == rn::e_land_overlay::hills );
  bool const has_mountains =
      ( square.overlay == rn::e_land_overlay::mountains );
  bool const has_mound = has_hills || has_mountains;
  bool const has_river = square.river.has_value();
  if( has_mound && !has_river ) {
    CHECK( square.overlay.has_value() );
    switch( *square.overlay ) {
      case rn::e_land_overlay::hills:
        // Hills and no river.
        res.hill_river = sav::hills_river_3bit_type::c;
        break;
      case rn::e_land_overlay::mountains:
        // Mountains and no river.
        res.hill_river = sav::hills_river_3bit_type::cc;
        break;
      case rn::e_land_overlay::forest:
        SHOULD_NOT_BE_HERE;
    }
  } else if( !has_mound && has_river ) {
    CHECK( square.river.has_value() );
    switch( *square.river ) {
      case rn::e_river::minor:
        // Minor river and no mound.
        res.hill_river = sav::hills_river_3bit_type::t;
        break;
      case rn::e_river::major:
        // Major river and no mound.
        res.hill_river = sav::hills_river_3bit_type::tt;
        break;
    }
  } else if( has_mound && has_river ) {
    if( has_mountains )
      return "The OG does not support rivers on mountains "
             "tiles.";
    if( square.river == rn::e_river::major )
      return "The OG does not support major rivers on tiles "
             "containing either mountains or hills.";
    // At this point all that we can be left with is hills and
    // minor river.
    CHECK_EQ( square.river, rn::e_river::minor );
    CHECK_EQ( square.overlay, rn::e_land_overlay::hills );
    res.hill_river = sav::hills_river_3bit_type::tc;
  }
  return res;
}

valid_or<std::string> tiles_to_map_squares(
    int og_map_size_x, int og_map_size_y,
    vector<sav::TILE> const& tiles, rn::RealTerrain& out ) {
  int const rn_map_size_x  = og_map_size_x - 2;
  int const rn_map_size_y  = og_map_size_y - 2;
  int const total_og_tiles = og_map_size_x * og_map_size_y;
  int const total_rn_tiles = rn_map_size_x * rn_map_size_y;
  if( og_map_size_x <= 2 || og_map_size_y <= 2 )
    return "map size too small";
  if( int( tiles.size() ) != total_og_tiles )
    return "inconsistent number of tiles";
  auto& map = out.map;

  vector<rn::MapSquare> squares;
  squares.reserve( total_rn_tiles );
  for( int y = 1; y < og_map_size_y - 1; ++y ) {
    for( int x = 1; x < og_map_size_x - 1; ++x ) {
      int const        og_offset = ( y * og_map_size_x ) + x;
      sav::TILE const& tile      = tiles[og_offset];
      UNWRAP_RETURN( square, map_square_from_tile( tile ) );
      squares.push_back( square );
    }
  }
  CHECK_EQ( int( squares.size() ), total_rn_tiles );
  map = gfx::Matrix<rn::MapSquare>( std::move( squares ),
                                    rn_map_size_x );
  return valid;
}

valid_or<std::string> map_squares_to_tiles(
    rn::RealTerrain const& in, uint16_t& og_map_size_x,
    uint16_t& og_map_size_y, vector<sav::TILE>& tiles ) {
  int const rn_map_size_x  = in.map.size().w;
  int const rn_map_size_y  = in.map.size().h;
  og_map_size_x            = rn_map_size_x + 2;
  og_map_size_y            = rn_map_size_y + 2;
  int const total_og_tiles = og_map_size_x * og_map_size_y;

  auto const& map = in.map;
  tiles.reserve( total_og_tiles );
  static sav::TILE kOcean{ .tile = sav::terrain_5bit_type::ttt };
  for( int y = 0; y < og_map_size_y; ++y ) {
    for( int x = 0; x < og_map_size_x; ++x ) {
      if( x == 0 || x == og_map_size_x - 1 || y == 0 ||
          y == og_map_size_y - 1 ) {
        tiles.push_back( kOcean );
        continue;
      }
      CHECK_GT( x, 0 );
      CHECK_GT( y, 0 );
      CHECK_LT( x, og_map_size_x - 1 );
      CHECK_LT( y, og_map_size_y - 1 );
      rn::Coord            coord{ .x = x - 1, .y = y - 1 };
      rn::MapSquare const& square = map[coord];
      UNWRAP_RETURN( tile, tile_from_map_square( square ) );
      tiles.push_back( tile );
    }
  }
  CHECK_EQ( int( tiles.size() ), total_og_tiles );
  return valid;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
valid_or<std::string> convert_to_rn( sav::ColonySAV const& in,
                                     rn::RootState& out ) {
  ScopedTimer timer( "convert saved game from OG to RN" );

  // Terrain.
  rn::wrapped::TerrainState terrain_o;
  HAS_VALUE_OR_RET( tiles_to_map_squares(
      in.header.map_size_x, in.header.map_size_y, in.tile,
      terrain_o.real_terrain ) );
  out.zzz_terrain = rn::TerrainState( std::move( terrain_o ) );

  // TODO: add more here.

  return valid;
}

valid_or<std::string> convert_to_og( rn::RootState const& in,
                                     sav::ColonySAV&      out ) {
  ScopedTimer timer( "convert saved game from RN to OG" );

  // Header.
  out.header.colonize = { 'C', 'O', 'L', 'O', 'N',
                          'I', 'Z', 'E', 0 };

  // Terrain.
  HAS_VALUE_OR_RET( map_squares_to_tiles(
      in.zzz_terrain.refl().real_terrain, out.header.map_size_x,
      out.header.map_size_y, out.tile ) );

  // TODO: add more here.

  return valid;
}

valid_or<std::string> convert_to_rn( sav::MapFile const& in,
                                     rn::RealTerrain&    out ) {
  ScopedTimer timer( "convert map from OG to RN" );
  HAS_VALUE_OR_RET( tiles_to_map_squares(
      in.map_size_x, in.map_size_y, in.tile, out ) );
  return valid;
}

valid_or<std::string> convert_to_og( rn::RealTerrain const& in,
                                     sav::ColonySAV& out ) {
  ScopedTimer timer( "convert map from RN to OG" );
  HAS_VALUE_OR_RET(
      map_squares_to_tiles( in, out.header.map_size_x,
                            out.header.map_size_y, out.tile ) );
  return valid;
}

} // namespace bridge
