/****************************************************************
**visibility.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-30.
*
* Description: Things related to hidden tiles and fog-of-war.
*
*****************************************************************/
#include "visibility.hpp"

// Revolution Now
#include "map-square.hpp"

// config
#include "config/colony.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

int largest_possible_sighting_radius() {
  static int const largest = [] {
    int res = 0;
    for( e_unit_type type : refl::enum_values<e_unit_type> )
      res = std::max( res, unit_attr( type ).visibility );
    return res;
  }();
  return largest;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
int unit_sight_radius( SSConst const& ss, e_nation nation,
                       e_unit_type type ) {
  UNWRAP_CHECK( player, ss.players.players[nation] );
  bool const has_de_soto =
      player.fathers.has[e_founding_father::hernando_de_soto];
  return unit_attr( type ).visibility + ( has_de_soto ? 1 : 0 );
}

vector<Coord> unit_visible_squares( SSConst const& ss,
                                    e_nation       nation,
                                    e_unit_type    type,
                                    Coord          tile ) {
  TerrainState const& terrain = ss.terrain;
  int const  radius = unit_sight_radius( ss, nation, type );
  Rect const possible =
      Rect::from( tile, Delta{ .w = 1, .h = 1 } )
          .with_border_added( radius );
  bool const    ship = unit_attr( type ).ship;
  vector<Coord> res;
  // 6x6 should be largest sighting block size in the game, which
  // will happen when e.g. a Galleon (whose radius is normally 2
  // -> 5x5) owned by a player with De Soto.
  int const largest_block_size =
      largest_possible_sighting_radius() * 2 + 1;
  res.reserve( largest_block_size * largest_block_size );
  for( Coord coord : possible ) {
    maybe<MapSquare const&> square =
        terrain.maybe_square_at( coord );
    if( !square.has_value() ) continue;
    if( abs( tile.x - coord.x ) > 1 ||
        abs( tile.y - coord.y ) > 1 ) {
      // The tiles beyond the immediately adjacent are only vis-
      // ible if their surface type matches that of the unit, re-
      // gardless of the unit's sight radius (the original game
      // does this).
      if( ship == is_land( *square ) ) continue;
    }
    res.push_back( coord );
  }
  return res;
}

refl::enum_map<e_nation, bool> nations_with_visibility_of_square(
    SSConst const& ss, Coord tile ) {
  refl::enum_map<e_nation, bool> res;
  // These are all the squares where there could possibly be a
  // unit that could see this square.
  Rect const possible_for_units =
      Rect::from( tile, Delta{ .w = 1, .h = 1 } )
          .with_border_added(
              largest_possible_sighting_radius() );
  for( Coord coord : possible_for_units ) {
    // We don't use the recursive variant because we don't want
    // e.g. a scout on a ship to increase the sighting radius.
    unordered_set<UnitId> const& units =
        ss.units.from_coord( coord );
    for( UnitId unit_id : units ) {
      Unit const& unit = ss.units.unit_for( unit_id );
      if( res[unit.nation()] )
        // If one unit on this square has a nation that can al-
        // ready see the tile in question then we can stop this
        // inner loop, because all of the other units on this
        // square (if any) will be the same nation.
        break;
      vector<Coord> const visible = unit_visible_squares(
          ss, unit.nation(), unit.type(), coord );
      if( find( visible.begin(), visible.end(), coord ) !=
          visible.end() ) {
        res[unit.nation()] = true;
        // Again, any other units on this tile will be from the
        // same nation, so no need to continue on this tile.
        break;
      }
    }
  }
  // These are all the squares where there could possibly be a
  // colony that could see this square. Assume that colonies have
  // a sighting radius of 1.
  Rect const possible_for_colonies =
      Rect::from( tile, Delta{ .w = 1, .h = 1 } )
          .with_border_added(
              config_colony.colony_visibility_radius );
  for( Coord coord : possible_for_colonies ) {
    maybe<ColonyId> const& colony_id =
        ss.colonies.maybe_from_coord( coord );
    if( !colony_id.has_value() ) continue;
    Colony const& colony = ss.colonies.colony_for( *colony_id );
    res[colony.nation]   = true;
  }
  return res;
}

} // namespace rn
