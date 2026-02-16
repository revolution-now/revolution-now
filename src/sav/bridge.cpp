/****************************************************************
**bridge.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-12.
*
* Description: Translates between the OG and NG save state
*              representation.
*
*****************************************************************/
#include "bridge.hpp"

// sav
#include "connectivity.hpp"
#include "map-file.hpp"
#include "sav-struct.hpp"

// ss
#include "ss/map-matrix.hpp"
#include "ss/nation.hpp"
#include "ss/player.rds.hpp"
#include "ss/root.hpp"
#include "ss/terrain-enums.rds.hpp"

// rds
#include "rds/switch-macro.hpp"

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
using ::base::maybe;
using ::base::nothing;
using ::base::ScopedTimer;
using ::base::valid;
using ::base::valid_or;
using ::refl::enum_values;

using ColonyIdToOg =
    base::function_ref<maybe<int>( ColonyId ) const>;

// TODO: replace this with std::to_underlying when C++23 becomes
// available.
template<typename E>
requires is_enum_v<E>
auto to_underlying( E e ) {
  return static_cast<underlying_type_t<E>>( e );
}

/****************************************************************
** General Helpers.
*****************************************************************/
[[nodiscard]] int get_ng_id( map<int, int> const& m,
                             int const og_id ) {
  if( m.empty() )
    // We're not in the mode where we are reverse mapping IDs, so
    // just us the OG ID + 1. We need to add 1 because the NG IDs
    // all start at 1.
    return og_id + 1;
  // We're in the mode where we are mapping IDs, thus the ID must
  // be present in the map.
  auto const iter = m.find( og_id );
  CHECK( iter != m.end(), "failed to look up OG ID {}", og_id );
  return iter->second;
}

struct OgHumanIndependence {
  rn::e_nation declared = {};
  rn::e_nation ref_slot = {};
};

struct OgIndependence {
  maybe<OgHumanIndependence> human;
  int ai_given_independence = 0;
};

expect<OgIndependence, string> independence_declared(
    sav::ColonySAV const& sav ) {
  OgIndependence ogi;

  bool const human_declared =
      sav.header.game_flags_1.independence_declared;

  int ai_granted = 0;

  // Find any AI players that were granted independence.
  if( sav.player[0].control == sav::control_type::ai &&
      sav.nation[0].nation_flags.granted_independence )
    ++ai_granted;
  if( sav.player[1].control == sav::control_type::ai &&
      sav.nation[1].nation_flags.granted_independence )
    ++ai_granted;
  if( sav.player[2].control == sav::control_type::ai &&
      sav.nation[2].nation_flags.granted_independence )
    ++ai_granted;
  if( sav.player[3].control == sav::control_type::ai &&
      sav.nation[3].nation_flags.granted_independence )
    ++ai_granted;

  ogi.ai_given_independence = ai_granted;

  if( !human_declared ) return ogi;

  maybe<rn::e_nation> human_that_declared;
  maybe<rn::e_nation> ref_slot; // iff human declared.

  int found_human_declared = 0;
  int found_ref_slot       = 0;

  // Find the human that declared.
  if( sav.player[0].control == sav::control_type::player ) {
    human_that_declared = rn::e_nation::english;
    ++found_human_declared;
  }
  if( sav.player[1].control == sav::control_type::player ) {
    human_that_declared = rn::e_nation::french;
    ++found_human_declared;
  }
  if( sav.player[2].control == sav::control_type::player ) {
    human_that_declared = rn::e_nation::spanish;
    ++found_human_declared;
  }
  if( sav.player[3].control == sav::control_type::player ) {
    human_that_declared = rn::e_nation::dutch;
    ++found_human_declared;
  }

  if( !human_that_declared.has_value() ||
      found_human_declared != 1 )
    return "independence has been declared but could not find "
           "precisely one human player.";
  CHECK( human_that_declared.has_value() );

  // Find the REF slot.
  if( sav.player[0].control == sav::control_type::ai ) {
    ref_slot = rn::e_nation::english;
    ++found_ref_slot;
  }
  if( sav.player[1].control == sav::control_type::ai ) {
    ref_slot = rn::e_nation::french;
    ++found_ref_slot;
  }
  if( sav.player[2].control == sav::control_type::ai ) {
    ref_slot = rn::e_nation::spanish;
    ++found_ref_slot;
  }
  if( sav.player[3].control == sav::control_type::ai ) {
    ref_slot = rn::e_nation::dutch;
    ++found_ref_slot;
  }

  if( !ref_slot.has_value() || found_ref_slot != 1 )
    return "independence has been declared but could not find "
           "precisely one REF slot.";
  CHECK( ref_slot.has_value() );

  auto& human    = ogi.human.emplace();
  human.declared = *human_that_declared;
  human.ref_slot = *ref_slot;

  return ogi;
}

sav::nation_2byte_type convert_to_og_nation_2byte_type(
    rn::e_nation const nation ) {
  switch( nation ) {
    case rn::e_nation::english:
      return sav::nation_2byte_type::england;
    case rn::e_nation::french:
      return sav::nation_2byte_type::france;
    case rn::e_nation::spanish:
      return sav::nation_2byte_type::spain;
    case rn::e_nation::dutch:
      return sav::nation_2byte_type::netherlands;
  }
}

// This is more non-trivial than it would first appear because if
// the player is an REF player then we need to find which OG na-
// tion it corresponds to.
expect<sav::nation_2byte_type, string>
convert_to_og_nation_2byte_type( sav::ColonySAV const& sav,
                                 rn::e_player const type ) {
  switch( type ) {
    case rn::e_player::english:
      return sav::nation_2byte_type::england;
    case rn::e_player::french:
      return sav::nation_2byte_type::france;
    case rn::e_player::spanish:
      return sav::nation_2byte_type::spain;
    case rn::e_player::dutch:
      return sav::nation_2byte_type::netherlands;
    case rn::e_player::ref_dutch:
    case rn::e_player::ref_english:
    case rn::e_player::ref_french:
    case rn::e_player::ref_spanish: {
      UNWRAP_RETURN_T( auto const nation_declared,
                       independence_declared( sav ) );
      if( !nation_declared.human.has_value() )
        // This should not happen because we should not be trying
        // convert an REF player pre-declaration.
        return format(
            "cannot convert REF player to OG nation "
            "pre-declaration." );
      return convert_to_og_nation_2byte_type(
          nation_declared.human->ref_slot );
    }
  }
}

rn::e_commodity og_cargo_4bit_to_ng(
    sav::cargo_4bit_type og_type ) {
  og_type = static_cast<sav::cargo_4bit_type>(
      to_underlying( og_type ) & 0b1111 );
  switch( og_type ) {
    case sav::cargo_4bit_type::food:
      return rn::e_commodity::food;
    case sav::cargo_4bit_type::sugar:
      return rn::e_commodity::sugar;
    case sav::cargo_4bit_type::tobacco:
      return rn::e_commodity::tobacco;
    case sav::cargo_4bit_type::cotton:
      return rn::e_commodity::cotton;
    case sav::cargo_4bit_type::furs:
      return rn::e_commodity::furs;
    case sav::cargo_4bit_type::lumber:
      return rn::e_commodity::lumber;
    case sav::cargo_4bit_type::ore:
      return rn::e_commodity::ore;
    case sav::cargo_4bit_type::silver:
      return rn::e_commodity::silver;
    case sav::cargo_4bit_type::horses:
      return rn::e_commodity::horses;
    case sav::cargo_4bit_type::rum:
      return rn::e_commodity::rum;
    case sav::cargo_4bit_type::cigars:
      return rn::e_commodity::cigars;
    case sav::cargo_4bit_type::cloth:
      return rn::e_commodity::cloth;
    case sav::cargo_4bit_type::coats:
      return rn::e_commodity::coats;
    case sav::cargo_4bit_type::goods:
      return rn::e_commodity::trade_goods;
    case sav::cargo_4bit_type::tools:
      return rn::e_commodity::tools;
    case sav::cargo_4bit_type::muskets:
      return rn::e_commodity::muskets;
  }
}

sav::cargo_4bit_type ng_commodity_to_og_cargo_4bit_type(
    rn::e_commodity ng_type ) {
  switch( ng_type ) {
    case rn::e_commodity::cigars:
      return sav::cargo_4bit_type::cigars;
    case rn::e_commodity::cloth:
      return sav::cargo_4bit_type::cloth;
    case rn::e_commodity::coats:
      return sav::cargo_4bit_type::coats;
    case rn::e_commodity::cotton:
      return sav::cargo_4bit_type::cotton;
    case rn::e_commodity::food:
      return sav::cargo_4bit_type::food;
    case rn::e_commodity::furs:
      return sav::cargo_4bit_type::furs;
    case rn::e_commodity::horses:
      return sav::cargo_4bit_type::horses;
    case rn::e_commodity::lumber:
      return sav::cargo_4bit_type::lumber;
    case rn::e_commodity::muskets:
      return sav::cargo_4bit_type::muskets;
    case rn::e_commodity::ore:
      return sav::cargo_4bit_type::ore;
    case rn::e_commodity::rum:
      return sav::cargo_4bit_type::rum;
    case rn::e_commodity::silver:
      return sav::cargo_4bit_type::silver;
    case rn::e_commodity::sugar:
      return sav::cargo_4bit_type::sugar;
    case rn::e_commodity::tobacco:
      return sav::cargo_4bit_type::tobacco;
    case rn::e_commodity::tools:
      return sav::cargo_4bit_type::tools;
    case rn::e_commodity::trade_goods:
      return sav::cargo_4bit_type::goods;
  }
}

maybe<rn::e_nation> og_nation_2byte_type_to_ng_nation(
    sav::nation_2byte_type const type ) {
  switch( type ) {
    case sav::nation_2byte_type::england:
      return rn::e_nation::english;
    case sav::nation_2byte_type::france:
      return rn::e_nation::french;
    case sav::nation_2byte_type::spain:
      return rn::e_nation::spanish;
    case sav::nation_2byte_type::netherlands:
      return rn::e_nation::dutch;
    case sav::nation_2byte_type::inca:
    case sav::nation_2byte_type::aztec:
    case sav::nation_2byte_type::arawak:
    case sav::nation_2byte_type::iroquois:
    case sav::nation_2byte_type::cherokee:
    case sav::nation_2byte_type::apache:
    case sav::nation_2byte_type::sioux:
    case sav::nation_2byte_type::tupi:
    case sav::nation_2byte_type::none:
      return nothing;
  }
  return nothing;
}

// Just in case there are multiple human players enabled which
// the NG supports. We need to select just one for the OG.
maybe<rn::e_nation> find_human_nation_ng(
    rn::RootState const& root ) {
  using namespace rn;
  maybe<e_nation> res;
  for( auto const& [type, player] : root.players.players ) {
    if( !player.has_value() ) continue;
    if( player->control == e_player_control::human ) {
      res = nation_for( type );
      break;
    }
  }
  return res;
}

maybe<rn::e_nation> find_human_player_og(
    sav::ColonySAV const& sav ) {
  CHECK_EQ( ssize( sav.player ), 4 );
  if( sav.player[0].control == sav::control_type::player )
    return rn::e_nation::english;
  if( sav.player[1].control == sav::control_type::player )
    return rn::e_nation::french;
  if( sav.player[2].control == sav::control_type::player )
    return rn::e_nation::spanish;
  if( sav.player[3].control == sav::control_type::player )
    return rn::e_nation::dutch;
  return nothing;
}

/****************************************************************
** Terrain.
*****************************************************************/
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

ConvResult tiles_to_map_squares( int og_map_size_x,
                                 int og_map_size_y,
                                 vector<sav::TILE> const& tiles,
                                 rn::RealTerrain& out ) {
  int const rn_map_size_x  = og_map_size_x - 2;
  int const rn_map_size_y  = og_map_size_y - 2;
  int const total_og_tiles = og_map_size_x * og_map_size_y;
  int const total_ng_tiles = rn_map_size_x * rn_map_size_y;
  if( og_map_size_x <= 2 || og_map_size_y <= 2 )
    return "map size too small";
  if( int( tiles.size() ) != total_og_tiles )
    return "inconsistent number of tiles";
  auto& map = out.map;

  vector<rn::MapSquare> squares;
  squares.reserve( total_ng_tiles );
  for( int y = 1; y < og_map_size_y - 1; ++y ) {
    for( int x = 1; x < og_map_size_x - 1; ++x ) {
      int const og_offset   = ( y * og_map_size_x ) + x;
      sav::TILE const& tile = tiles[og_offset];
      UNWRAP_RETURN( square, map_square_from_tile( tile ) );
      squares.push_back( square );
    }
  }
  CHECK_EQ( int( squares.size() ), total_ng_tiles );
  map = rn::MapMatrix( std::move( squares ), rn_map_size_x );
  return valid;
}

ConvResult map_squares_to_tiles( rn::RealTerrain const& in,
                                 uint16_t& og_map_size_x,
                                 uint16_t& og_map_size_y,
                                 vector<sav::TILE>& tiles ) {
  int const rn_map_size_x  = in.map.size().w;
  int const rn_map_size_y  = in.map.size().h;
  og_map_size_x            = rn_map_size_x + 2;
  og_map_size_y            = rn_map_size_y + 2;
  int const total_og_tiles = og_map_size_x * og_map_size_y;

  auto const& map = in.map;
  tiles.clear();
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
      rn::Coord coord{ .x = x - 1, .y = y - 1 };
      rn::MapSquare const& square = map[coord];
      UNWRAP_RETURN( tile, tile_from_map_square( square ) );
      tiles.push_back( tile );
    }
  }
  CHECK_EQ( int( tiles.size() ), total_og_tiles );
  return valid;
}

ConvResult populate_pacific_ocean_endpoints(
    sav::ColonySAV const& in, rn::wrapped::TerrainState& out ) {
  int const ng_map_height = out.real_terrain.map.size().h;
  int const og_map_height = in.header.map_size_y - 2;
  if( og_map_height != ng_map_height )
    return format( "inconsistent map heights: {} != {}",
                   og_map_height, ng_map_height );

  out.pacific_ocean_endpoints.resize( ng_map_height );

  // TODO: populate pacific ocean endpoints.

  return valid;
}

ConvResult populate_player_terrain(
    sav::ColonySAV const& in, rn::PlayersState const& ng_players,
    rn::wrapped::TerrainState& out ) {
  UNWRAP_RETURN_T( auto const declared,
                   independence_declared( in ) );

  auto const ng_ref_player = [&] -> maybe<rn::Player const&> {
    if( !declared.human.has_value() ) return nothing;
    rn::e_player const ng_ref_player_type =
        rn::ref_player_for( declared.human->declared );
    // The REF player should have already been created.
    CHECK( ng_players.players[ng_ref_player_type].has_value() );
    return *ng_players.players[ng_ref_player_type];
  }();

  auto const create_ng_player_terrain =
      [&]( rn::e_player const type ) -> rn::PlayerTerrain& {
    CHECK( !out.player_terrain[type].has_value() );
    auto& pterrain = out.player_terrain[type].emplace();
    pterrain.map   = gfx::Matrix<rn::PlayerSquare>(
        out.real_terrain.map.size() );
    return pterrain;
  };

  create_ng_player_terrain( rn::e_player::english );
  create_ng_player_terrain( rn::e_player::french );
  create_ng_player_terrain( rn::e_player::spanish );
  create_ng_player_terrain( rn::e_player::dutch );

  if( ng_ref_player.has_value() )
    create_ng_player_terrain( ng_ref_player->type );

  // TODO: populate player terrain.

  return valid;
}

/****************************************************************
** Colonies.
*****************************************************************/
// The assumption here is that when we are interconverting
// colonies state we will keep the colonies in the same order.
maybe<ColonyId> og_colony_id_to_ng(
    rn::ColoniesState const& /*ng_colonies*/,
    uint16_t const og_colony_index ) {
  // In the NG, colony IDs start at 1.
  ColonyId const attempt_ng_colony_id = og_colony_index + 1;
  // FIXME: enable this once we are converting colonies.
#if 0
  if( !ng_colonies.exists( attempt_ng_colony_id ) )
    return nothing;
#endif
  return attempt_ng_colony_id;
}

/****************************************************************
** Trade Routes.
*****************************************************************/
maybe<rn::e_trade_route_type> trade_route_type_to_ng(
    sav::trade_route_type const og_type ) {
  switch( og_type ) {
    case sav::trade_route_type::land:
      return rn::e_trade_route_type::land;
    case sav::trade_route_type::sea:
      return rn::e_trade_route_type::sea;
  }
  // Note that these OG enums are not guaranteed to have only the
  // values in our C++ enum, thus we need to handle getting here.
  return nothing;
}

sav::trade_route_type trade_route_type_to_og(
    rn::e_trade_route_type const ng_type ) {
  switch( ng_type ) {
    case rn::e_trade_route_type::land:
      return sav::trade_route_type::land;
    case rn::e_trade_route_type::sea:
      return sav::trade_route_type::sea;
  }
}

ConvResult convert_trade_routes_to_ng(
    sav::ColonySAV const& sav, rn::e_player const human,
    rn::ColoniesState const& ng_colonies,
    rn::TradeRouteState& out, IdMap const& id_map ) {
  out.routes.clear();
  for( int idx = 0;
       sav::TRADEROUTE const& og_route : sav.trade_route ) {
    if( idx >= sav.header.trade_route_count ) break;
    int const ng_id = get_ng_id( id_map.trade_route_ids, idx );
    out.last_trade_route_id =
        std::max( out.last_trade_route_id, ng_id );
    rn::TradeRoute& ng_route =
        out.routes[out.last_trade_route_id];
    ng_route.id     = out.last_trade_route_id;
    ng_route.name   = base::to_str( og_route.name );
    ng_route.player = human;
    auto const ng_type =
        trade_route_type_to_ng( og_route.land_or_sea );
    if( !ng_type.has_value() )
      return format(
          "unrecognized OG trade route type 0x{:02x}.",
          to_underlying( og_route.land_or_sea ) );
    ng_route.type = *ng_type;
    for( int stop_idx = 0;
         stop_idx < int( og_route.stops_count ); ++stop_idx ) {
      rn::TradeRouteStop& ng_stop =
          ng_route.stops.emplace_back();
      CHECK_LT( stop_idx, ssize( og_route.stops ) );
      sav::Stops const& og_stop = og_route.stops[stop_idx];
      if( og_stop.colony_index == 999 )
        ng_stop.target = rn::TradeRouteTarget::harbor{};
      else {
        auto const ng_colony_id = og_colony_id_to_ng(
            ng_colonies, og_stop.colony_index );
        if( !ng_colony_id.has_value() )
          return "trade route colony index {} not found.";
        ng_stop.target = rn::TradeRouteTarget::colony{
          .colony_id = *ng_colony_id };
      }
      int const n_loads =
          og_stop.loads_and_unloads_count.loads_count;
      int const n_unloads =
          og_stop.loads_and_unloads_count.unloads_count;

      if( n_loads > 0 )
        ng_stop.loads.push_back(
            og_cargo_4bit_to_ng( og_stop.loads_cargo.cargo_1 ) );
      if( n_loads > 1 )
        ng_stop.loads.push_back(
            og_cargo_4bit_to_ng( og_stop.loads_cargo.cargo_2 ) );
      if( n_loads > 2 )
        ng_stop.loads.push_back(
            og_cargo_4bit_to_ng( og_stop.loads_cargo.cargo_3 ) );
      if( n_loads > 3 )
        ng_stop.loads.push_back(
            og_cargo_4bit_to_ng( og_stop.loads_cargo.cargo_4 ) );
      if( n_loads > 4 )
        ng_stop.loads.push_back(
            og_cargo_4bit_to_ng( og_stop.loads_cargo.cargo_5 ) );
      if( n_loads > 5 )
        ng_stop.loads.push_back(
            og_cargo_4bit_to_ng( og_stop.loads_cargo.cargo_6 ) );

      if( n_unloads > 0 )
        ng_stop.unloads.push_back( og_cargo_4bit_to_ng(
            og_stop.unloads_cargo.cargo_1 ) );
      if( n_unloads > 1 )
        ng_stop.unloads.push_back( og_cargo_4bit_to_ng(
            og_stop.unloads_cargo.cargo_2 ) );
      if( n_unloads > 2 )
        ng_stop.unloads.push_back( og_cargo_4bit_to_ng(
            og_stop.unloads_cargo.cargo_3 ) );
      if( n_unloads > 3 )
        ng_stop.unloads.push_back( og_cargo_4bit_to_ng(
            og_stop.unloads_cargo.cargo_4 ) );
      if( n_unloads > 4 )
        ng_stop.unloads.push_back( og_cargo_4bit_to_ng(
            og_stop.unloads_cargo.cargo_5 ) );
      if( n_unloads > 5 )
        ng_stop.unloads.push_back( og_cargo_4bit_to_ng(
            og_stop.unloads_cargo.cargo_6 ) );

      CHECK_EQ( ssize( ng_stop.loads ), n_loads );
      CHECK_EQ( ssize( ng_stop.unloads ), n_unloads );
    }
    ++idx;
  }
  return valid;
}

ConvResult convert_trade_routes_to_og(
    rn::RootState const& root, rn::Player const& human,
    ColonyIdToOg const colony_id_to_og, sav::ColonySAV& out,
    IdMap& id_map ) {
  bool const has_foreign_routes = [&] {
    for( auto const& [id, route] : root.trade_routes.routes )
      if( route.player != human.type ) //
        return true;
    return false;
  }();

  if( has_foreign_routes )
    return "The OG does not support trade routes owned by "
           "players other than the main human player.";

  int const num_routes = root.trade_routes.routes.size();

  if( num_routes > 12 )
    return format(
        "The OG only supports a maximum of 12 trade routes. "
        "There are {} trade routes which exceeds the limit.",
        num_routes );
  out.header.trade_route_count = num_routes;
  for( int idx = 0;
       auto const& [id, ng_route] : root.trade_routes.routes ) {
    id_map.trade_route_ids[idx] = id;
    if( ng_route.stops.size() > 4 )
      return format(
          "The OG supports a maximum of four stops per trade "
          "route, but there is one with {} stops.",
          ng_route.stops.size() );
    // Should have been checked above.
    CHECK_LT( idx, 12 );
    CHECK_EQ( ng_route.player, human.type );
    CHECK_LE( ssize( ng_route.stops ), 4 );
    auto& og_route = out.trade_route[idx];
    if( ng_route.name.size() > og_route.name.a.size() )
      return format(
          "The OG supports trade route names of maximum length "
          "{}, but encountered one of length {}.",
          og_route.name.a.size(), ng_route.name.size() );
    CHECK( og_route.name.populate_from_string( ng_route.name ) );
    og_route.land_or_sea =
        trade_route_type_to_og( ng_route.type );
    og_route.stops_count = ng_route.stops.size();
    auto& og_stops       = og_route.stops;
    auto& ng_stops       = ng_route.stops;
    for( int stop_idx = 0;
         rn::TradeRouteStop const& ng_stop : ng_stops ) {
      // Should have been checked above.
      CHECK_LT( stop_idx, 4 );
      auto& og_stop = og_stops[stop_idx];
      SWITCH( ng_stop.target ) {
        CASE( colony ) {
          auto const og_colony_id =
              colony_id_to_og( colony.colony_id );
          if( !og_colony_id.has_value() )
            return format(
                "Could not map NG colony ID {} to OG colony ID.",
                colony.colony_id );
          og_stop.colony_index = *og_colony_id;
          break;
        }
        CASE( harbor ) {
          og_stop.colony_index = 999;
          break;
        }
      }
      if( ng_stop.loads.size() > 6 )
        return format(
            "The OG supports trade route commodity load counts "
            "of up to six, but there is one ({}) that loads "
            "{}.",
            ng_route.name, ng_stop.loads.size() );
      if( ng_stop.unloads.size() > 6 )
        return format(
            "The OG supports trade route commodity unload "
            "counts of up to six, but there is one ({}) that "
            "loads {}.",
            ng_route.name, ng_stop.unloads.size() );
      og_stop.loads_and_unloads_count.loads_count =
          ng_stop.loads.size();
      og_stop.loads_and_unloads_count.unloads_count =
          ng_stop.unloads.size();
      if( ng_stop.loads.size() > 0 )
        og_stop.loads_cargo.cargo_1 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.loads[0] );
      if( ng_stop.loads.size() > 1 )
        og_stop.loads_cargo.cargo_2 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.loads[1] );
      if( ng_stop.loads.size() > 2 )
        og_stop.loads_cargo.cargo_3 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.loads[2] );
      if( ng_stop.loads.size() > 3 )
        og_stop.loads_cargo.cargo_4 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.loads[3] );
      if( ng_stop.loads.size() > 4 )
        og_stop.loads_cargo.cargo_5 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.loads[4] );
      if( ng_stop.loads.size() > 5 )
        og_stop.loads_cargo.cargo_6 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.loads[5] );

      if( ng_stop.unloads.size() > 0 )
        og_stop.unloads_cargo.cargo_1 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.unloads[0] );
      if( ng_stop.unloads.size() > 1 )
        og_stop.unloads_cargo.cargo_2 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.unloads[1] );
      if( ng_stop.unloads.size() > 2 )
        og_stop.unloads_cargo.cargo_3 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.unloads[2] );
      if( ng_stop.unloads.size() > 3 )
        og_stop.unloads_cargo.cargo_4 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.unloads[3] );
      if( ng_stop.unloads.size() > 4 )
        og_stop.unloads_cargo.cargo_5 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.unloads[4] );
      if( ng_stop.unloads.size() > 5 )
        og_stop.unloads_cargo.cargo_6 =
            ng_commodity_to_og_cargo_4bit_type(
                ng_stop.unloads[5] );

      ++stop_idx;
    }
    ++idx;
  }
  return valid;
}

/****************************************************************
** Land View.
*****************************************************************/
ConvResult convert_landview_to_ng( sav::ColonySAV const& sav,
                                   rn::LandViewState& out ) {
  switch( sav.stuff.zoom_level ) {
    case 0:
      out.viewport.zoom = 1.0;
      break;
    case 1:
      out.viewport.zoom = 0.5;
      break;
    case 2:
      out.viewport.zoom = 0.25;
      break;
    case 3:
      out.viewport.zoom = 0.125;
      break;
    default:
      out.viewport.zoom = 1.0;
      break;
  }
  out.viewport.center_x = ( sav.stuff.viewport_x - 1 ) * 32;
  out.viewport.center_y = ( sav.stuff.viewport_y - 1 ) * 32;

  out.white_box.x = sav.stuff.white_box_x - 1;
  out.white_box.y = sav.stuff.white_box_y - 1;

  out.minimap.origin = {}; // TODO: needed?

  if( sav.header.show_entire_map )
    out.map_revealed = rn::MapRevealed::entire{};
  else {
    if( sav.header.fixed_nation_map_view ==
        sav::nation_2byte_type::none ) {
      out.map_revealed = rn::MapRevealed::no_special_view{};
    } else {
      auto const nation = og_nation_2byte_type_to_ng_nation(
          sav.header.fixed_nation_map_view );
      if( !nation.has_value() )
        return format(
            "unrecognized fixed_nation_map_view value: {}",
            sav.header.fixed_nation_map_view );
      out.map_revealed = rn::MapRevealed::player{
        .type = rn::colonial_player_for( *nation ) };
    }
  }
  return valid;
}

ConvResult convert_landview_to_og( rn::LandViewState const& in,
                                   sav::ColonySAV& out ) {
  if( in.viewport.zoom >= .75 )
    out.stuff.zoom_level = 0;
  else if( in.viewport.zoom >= .37 )
    out.stuff.zoom_level = 1;
  else if( in.viewport.zoom >= .19 )
    out.stuff.zoom_level = 2;
  else
    out.stuff.zoom_level = 3;

  if( out.header.map_size_x == 0 || out.header.map_size_y == 0 )
    return "The OG map size must be populated before converting "
           "the land view state.";

  out.stuff.viewport_x =
      clamp( int( floor( in.viewport.center_x / 32.0 ) + 1 ), 1,
             int( out.header.map_size_x - 1 ) );
  out.stuff.viewport_y =
      clamp( int( floor( in.viewport.center_y / 32.0 ) + 1 ), 1,
             int( out.header.map_size_y - 1 ) );

  out.stuff.white_box_x = in.white_box.x + 1;
  out.stuff.white_box_y = in.white_box.y + 1;

  SWITCH( in.map_revealed ) {
    CASE( no_special_view ) {
      out.header.show_entire_map = 0;
      out.header.fixed_nation_map_view =
          sav::nation_2byte_type::none;
      break;
    }
    CASE( entire ) {
      out.header.show_entire_map = 1;
      out.header.fixed_nation_map_view =
          sav::nation_2byte_type::none;
      break;
    }
    CASE( player ) {
      out.header.show_entire_map = 0;
      UNWRAP_RETURN_T( sav::nation_2byte_type const nation,
                       convert_to_og_nation_2byte_type(
                           as_const( out ), player.type ) );
      out.header.fixed_nation_map_view = nation;
      break;
    }
  }
  return valid;
}

/****************************************************************
** Players.
*****************************************************************/
ConvResult convert_players_to_ng( sav::ColonySAV const& in,
                                  rn::PlayersState& out ) {
  out.players = {};

  auto const create_ng_player =
      [&]( rn::e_player const type ) -> rn::Player& {
    CHECK( !out.players[type].has_value() );
    rn::Player& pl = out.players[type].emplace();
    pl.type        = type;
    pl.nation      = rn::nation_for( type );
    return pl;
  };

  UNWRAP_RETURN_T( auto const declared,
                   independence_declared( in ) );

  auto const ng_ref_player = [&] -> maybe<rn::Player&> {
    if( !declared.human.has_value() ) return nothing;
    rn::e_player const ng_ref_player_type =
        rn::ref_player_for( declared.human->declared );
    CHECK( !out.players[ng_ref_player_type].has_value() );
    return create_ng_player( ng_ref_player_type );
  }();

  auto const set_control = []( rn::Player& ng,
                               sav::PLAYER const& og ) {
    ng.control = rn::e_player_control::inactive;
    switch( og.control ) {
      case sav::control_type::withdrawn:
        ng.control = rn::e_player_control::inactive;
        break;
      case sav::control_type::player:
        ng.control = rn::e_player_control::human;
        break;
      case sav::control_type::ai:
        ng.control = rn::e_player_control::ai;
        break;
    }
  };

  // In the OG all four players will have values; though some may
  // be withdrawn. NOTE: after independence is declared, one of
  // the players will be the REF. So the strategy here is to just
  // create all four colonial players and set them to inactive.
  // Then decide, based on whether we are pre-declaration or
  // post) which ones to enable and how.
  rn::Player& english =
      create_ng_player( rn::e_player::english );
  rn::Player& french = create_ng_player( rn::e_player::french );
  rn::Player& spanish =
      create_ng_player( rn::e_player::spanish );
  rn::Player& dutch = create_ng_player( rn::e_player::dutch );

  // Convert player presence/control.
  CHECK_EQ( ssize( in.player ), 4 );
  sav::PLAYER const& og_p0 = in.player[0];
  sav::PLAYER const& og_p1 = in.player[1];
  sav::PLAYER const& og_p2 = in.player[2];
  sav::PLAYER const& og_p3 = in.player[3];
  set_control( english, og_p0 );
  set_control( french, og_p1 );
  set_control( spanish, og_p2 );
  set_control( dutch, og_p3 );

  if( ng_ref_player.has_value() )
    ng_ref_player->control = rn::e_player_control::ai;

  // TODO: add more here.

  return valid;
}

ConvResult convert_players_to_og( rn::RootState const& in,
                                  rn::Player const& human,
                                  sav::ColonySAV& out ) {
  auto const succession_done = in.events.war_of_succession_done;

  if( human.revolution.rebel_sentiment >= 50 &&
      !succession_done.has_value() )
    return "rebel sentiment is >= 50 but the war of succession "
           "has not been done, possibly due to config settings. "
           " Such a game cannot be correctly converted.";

  auto const set_og_player_control =
      [&]( rn::e_nation const nation,
           sav::control_type const ctype ) {
        CHECK_LT( static_cast<int>( nation ), 4 );
        out.player[static_cast<int>( nation )].control = ctype;
      };

  // Check colonial players.
  for( rn::e_nation const nation : enum_values<rn::e_nation> ) {
    auto const& ng_player =
        in.players.players[rn::colonial_player_for( nation )];
    set_og_player_control( nation,
                           sav::control_type::withdrawn );
    if( !ng_player.has_value() )
      // Leave it as withdrawn for now, then maybe later we'll
      // come back to it and enable it if it is the slot used for
      // an REF player.
      continue;
    switch( ng_player->control ) {
      case rn::e_player_control::inactive:
        // Leave it as withdrawn for now, then maybe later we'll
        // come back to it and enable it if it is the slot used
        // for an REF player.
        break;
      case rn::e_player_control::human:
        set_og_player_control( nation,
                               sav::control_type::player );
        break;
      case rn::e_player_control::ai:
        set_og_player_control( nation, sav::control_type::ai );
        break;
    }
  }

  // Check REF players.
  for( bool found_ref = false;
       rn::e_nation const nation : enum_values<rn::e_nation> ) {
    auto const& ng_ref_player =
        in.players.players[rn::ref_player_for( nation )];
    if( !ng_ref_player.has_value() ) continue;
    if( found_ref )
      return "Multiple REF players are not compatible with the "
             "OG.";
    found_ref = true;
    switch( ng_ref_player->control ) {
      case rn::e_player_control::inactive:
        // REF has lost. In the OG this is still AI.
        set_og_player_control( nation, sav::control_type::ai );
        break;
      case rn::e_player_control::ai:
        // REF is still active.
        set_og_player_control( nation, sav::control_type::ai );
        break;
      case rn::e_player_control::human:
        return "Human-controlled REF players are not compatible "
               "with the OG.";
    }
  }

  // TODO: add more here.

  return valid;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
ConvResult convert_to_ng( sav::ColonySAV const& in,
                          rn::RootState& out,
                          IdMap const& id_map ) {
  ScopedTimer timer( "convert saved game from OG to RN" );

  auto const human_nation = find_human_player_og( in );
  if( !human_nation.has_value() )
    return "Cannot find any players under human control.";
  rn::e_player const player_type =
      rn::colonial_player_for( *human_nation );

  // Header.
  // TODO

  // Players.
  GOOD_OR_RETURN( convert_players_to_ng( in, out.players ) );

  // Terrain.
  rn::wrapped::TerrainState terrain_o;
  GOOD_OR_RETURN( tiles_to_map_squares(
      in.header.map_size_x, in.header.map_size_y, in.tile,
      terrain_o.real_terrain ) );
  GOOD_OR_RETURN(
      populate_pacific_ocean_endpoints( in, terrain_o ) );
  GOOD_OR_RETURN( populate_player_terrain(
      in, as_const( out.players ), terrain_o ) );
  out.zzz_terrain = rn::TerrainState( std::move( terrain_o ) );

  // Colonies.
  // TODO

  // Trade Routes.
  GOOD_OR_RETURN(
      convert_trade_routes_to_ng( in, player_type, out.colonies,
                                  out.trade_routes, id_map ) );

  // Land view.
  GOOD_OR_RETURN( convert_landview_to_ng( in, out.land_view ) );

  // TODO: for the algo to convert the prime resource depletion
  // counters, see doc/depletion.txt.

  // TODO: add more here.

  {
    // TODO: not sure yet if we're going to keep this here or
    // just have the caller do it.
    ScopedTimer const timer( "full game state validation" );
    GOOD_OR_RETURN( validate_recursive( as_const( out ) ) );
  }

  return valid;
}

ConvResult convert_to_og( rn::RootState const& in,
                          sav::ColonySAV& out, IdMap& id_map ) {
  ScopedTimer timer( "convert saved game from RN to OG" );

  auto const human_nation = find_human_nation_ng( in );
  if( !human_nation.has_value() )
    return "There must be at least one human player enabled.";
  rn::e_player const ng_human_player_type =
      rn::colonial_player_for( *human_nation );
  if( !in.players.players[ng_human_player_type].has_value() )
    return format(
        "failed to look up player object for human player {}",
        ng_human_player_type );
  rn::Player const& ng_human_player =
      *in.players.players[ng_human_player_type];

  // Header.
  // TODO: move this to a dedicated function.
  out.header.colonize = { 'C', 'O', 'L', 'O', 'N',
                          'I', 'Z', 'E', 0 };
  bool const declared = ng_human_player.revolution.status >=
                        rn::e_revolution_status::declared;
  out.header.game_flags_1.independence_declared = declared;

  // Players.
  GOOD_OR_RETURN(
      convert_players_to_og( in, ng_human_player, out ) );

  // Terrain.
  GOOD_OR_RETURN( map_squares_to_tiles(
      in.zzz_terrain.refl().real_terrain, out.header.map_size_x,
      out.header.map_size_y, out.tile ) );

  // Colonies.
  map<ColonyId, int /*og colony idx*/> colony_id_map;
  auto const colony_id_map_fn =
      [&]( ColonyId const ng_colony_id ) -> maybe<int> {
  // FIXME: re-enable this after we convert colonies.
#if 0
    auto const iter = colony_id_map.find( ng_colony_id );
    if( iter == colony_id_map.end() ) return nothing;
    return iter->second;
#else
    CHECK_GT( ng_colony_id, 0 );
    return ng_colony_id - 1;
#endif
  };

  // Trade Routes.
  // TODO: sanitize the trade routes here first, since there is
  // not a guarantee that will have happened before we do this,
  // and that will make it more likely that things will be kosher
  // and not upset the OG with what we write.
  GOOD_OR_RETURN( convert_trade_routes_to_og(
      in, ng_human_player, colony_id_map_fn, out, id_map ) );

  // Land view.
  GOOD_OR_RETURN( convert_landview_to_og( in.land_view, out ) );

  // TODO: This only populates the region IDs; we need to popu-
  // late the visitor_nation field separately.
  populate_region_ids( as_const( out ).tile, out.path );

  CHECK_GT( out.header.map_size_x, 0 );
  populate_connectivity(
      as_const( out ).tile, as_const( out ).path,
      { .w = out.header.map_size_x, .h = out.header.map_size_y },
      out.connectivity );

  // TODO: for the algo to convert the prime resource depletion
  // counters, see doc/depletion.txt.

  // TODO: add more here.

  return valid;
}

ConvResult convert_map_to_ng( sav::MapFile const& in,
                              rn::RealTerrain& out ) {
  ScopedTimer timer( "convert map from OG to RN" );
  GOOD_OR_RETURN( tiles_to_map_squares(
      in.map_size_x, in.map_size_y, in.tile, out ) );
  return valid;
}

ConvResult convert_map_to_og( rn::RealTerrain const& in,
                              sav::MapFile& out ) {
  ScopedTimer timer( "convert map from RN to OG" );
  sav::HEADER header;
  GOOD_OR_RETURN( map_squares_to_tiles(
      in, header.map_size_x, header.map_size_y, out.tile ) );

  // This only populates the region IDs, but that is fine for a
  // pure map conversion.
  populate_region_ids( as_const( out ).tile, out.path );

  return valid;
}

} // namespace bridge
