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
#include "imap-updater.hpp"
#include "land-view.hpp"
#include "map-square.hpp"
#include "plane-stack.hpp"
#include "ts.hpp"

// config
#include "config/fathers.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/iter.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/timer.hpp"

using namespace std;

namespace rn {

namespace {

int largest_possible_sighting_radius() {
  static int const largest = [] {
    int res = 0;
    for( e_unit_type type : refl::enum_values<e_unit_type> )
      res = std::max( res, unit_attr( type ).visibility );
    // +1 in case a player has de soto.
    return res + 1;
  }();
  return largest;
}

// The unit's site radius is 1 for most units and two for scouts
// and some ships, and then having De Soto gives a +1 to all
// units. In the OG ships don't get the De Soto bonus, but in
// this game we do give it to ships. A radius of 1 means 3x3
// visibility, while a radius of 2 means 5x5 visibility, etc.
int unit_sight_radius( SSConst const& ss, e_nation nation,
                       e_unit_type type ) {
  UNWRAP_CHECK( player, ss.players.players[nation] );
  bool const has_de_soto =
      player.fathers.has[e_founding_father::hernando_de_soto];
  int visibility = unit_attr( type ).visibility;
  if( has_de_soto ) {
    if( !unit_attr( type ).ship )
      ++visibility;
    else if( config_fathers.rules
                 .ships_get_de_soto_sighting_bonus )
      // The original games that De Soto will give all units an
      // extended sighting radius. The meaning of this apparently
      // is that it takes the sighting radius of each unit and
      // adds one to it (even if it was already 2), since that is
      // what it does for land units. However, it does not do
      // this for ships at all. In this game do default to giving
      // the bonus to ships in order to make De Soto a bit more
      // useful, but that can be safely turned off here if the
      // OG's behavior is desired.
      ++visibility;
  }
  return visibility;
}

} // namespace

/****************************************************************
** Visibility
*****************************************************************/
Visibility::Visibility( SSConst const&  ss,
                        maybe<e_nation> nation )
  : terrain_( &ss.terrain ),
    nation_( nation ),
    player_terrain_(
        ( nation.has_value()
              ? ss.terrain.player_terrain( *nation )
              : base::nothing )
            .fmap( []( auto& arg ) { return &arg; } ) ) {}

e_tile_visibility Visibility::visible( Coord tile ) const {
  DCHECK( terrain_ != nullptr );
  if( !player_terrain_.has_value() )
    // No player, so always visible.
    return e_tile_visibility::visible_and_clear;
  DCHECK( *player_terrain_ != nullptr );
  // We're rendering from the player's point of view.
  if( !tile.is_inside( terrain_->world_rect_tiles() ) )
    // Proto squares are never considered visible.
    return e_tile_visibility::hidden;
  DCHECK( player_terrain_.has_value() );
  maybe<FogSquare> const& fog_square =
      ( *player_terrain_ )->map[tile];
  if( !fog_square.has_value() )
    // There is a player and they can't see this tile.
    return e_tile_visibility::hidden;
  if( !fog_square->fog_of_war_removed )
    // There is a player, they've seen this tile, but no units
    // can currently see it, so it is fogged.
    return e_tile_visibility::visible_with_fog;
  // There is a player and they can currently see this tile.
  return e_tile_visibility::visible_and_clear;
}

MapSquare const& Visibility::square_at( Coord tile ) const {
  DCHECK( terrain_ != nullptr );
  if( !player_terrain_.has_value() )
    // Real map.
    return terrain_->total_square_at( tile );
  DCHECK( *player_terrain_ != nullptr );
  // We're rendering from the player's point of view.
  if( !tile.is_inside( terrain_->world_rect_tiles() ) )
    // Will yield a proto square.
    return terrain_->total_square_at( tile );
  maybe<FogSquare> const& fog_square =
      ( *player_terrain_ )->map[tile];
  if( !fog_square.has_value() )
    // Player can't see this tile (it is hidden).
    return terrain_->total_square_at( tile );
  // The player can see this tile, either clear or fogged.
  if( fog_square->fog_of_war_removed )
    // Use the real tile in this case because the fogged tile is
    // not always guaranteed to match the real tile when the tile
    // is visible, since the fog square is only updated in re-
    // sopnse to certain actions (and that's ok, because when a
    // tile is visible and clear, the fog square is not used at
    // all for rendering).
    return terrain_->total_square_at( tile );
  // Return the player's version of it.
  return fog_square->square;
};

maybe<FogSquare const&> Visibility::fog_square_at(
    Coord tile ) const {
  DCHECK( terrain_ != nullptr );
  if( !player_terrain_.has_value() )
    // Real map, no fog.
    return nothing;
  DCHECK( *player_terrain_ != nullptr );
  // We're rendering from the player's point of view.
  if( !tile.is_inside( terrain_->world_rect_tiles() ) )
    // Would be a proto square; again, not fog square.
    return nothing;
  maybe<FogSquare> const& square =
      ( *player_terrain_ )->map[tile];
  if( !square.has_value() )
    // Player can't see this tile.
    return nothing;
  // The player can see this tile, so return the player's version
  // of it.
  return *square;
}

Rect Visibility::rect_tiles() const {
  DCHECK( terrain_ != nullptr );
  return terrain_->world_rect_tiles();
}

bool Visibility::on_map( Coord tile ) const {
  return tile.is_inside( rect_tiles() );
}

/****************************************************************
** Public API
*****************************************************************/
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
  for( Rect rect : gfx::subrects( possible ) ) {
    Coord                   coord = rect.upper_left();
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

bool does_nation_have_fog_removed_on_square( SSConst const& ss,
                                             e_nation nation,
                                             Coord    tile ) {
  if( !ss.players.players[nation].has_value() ) return false;
  UNWRAP_CHECK( player_terrain,
                ss.terrain.player_terrain( nation ) );
  maybe<FogSquare> const& fog_square = player_terrain.map[tile];
  return fog_square.has_value() &&
         fog_square->fog_of_war_removed;
}

void recompute_fog_for_nation( SS& ss, TS& ts,
                               e_nation nation ) {
  base::ScopedTimer timer(
      fmt::format( "regenerating fog-of-war ({})", nation ) );
  timer.options().no_checkpoints_logging = true;

  UNWRAP_CHECK( player_terrain,
                ss.terrain.player_terrain( nation ) );
  gfx::Matrix<maybe<FogSquare>> const& m = player_terrain.map;

  timer.checkpoint( "generate fogged set" );
  unordered_set<Coord> fogged;
  for( int y = 0; y < m.size().h; ++y ) {
    for( int x = 0; x < m.size().w; ++x ) {
      Coord const             coord{ .x = x, .y = y };
      maybe<FogSquare> const& fog_square = m[coord];
      if( !fog_square.has_value() ) continue;
      if( !fog_square->fog_of_war_removed )
        // There is already fog.
        continue;
      fogged.insert( coord );
    }
  }

  // Unfog the surroundings of units.
  //
  // FIXME: it's not ideal that we iterate over all euro units
  // here because that includes the ones working in colonies that
  // we don't want. It also would not be ideal to iterate over
  // all units on the map because that would include all of the
  // braves walking around which we don't want. Ideally we need
  // to add a cache for euro units on the map, then we can it-
  // erate over that only.
  timer.checkpoint( "de-fog unit surroundings" );
  for( auto& [unit_id, p_state] : ss.units.euro_all() ) {
    maybe<UnitOwnership::world const&> world =
        p_state->ownership.get_if<UnitOwnership::world>();
    if( !world.has_value() ) continue;
    Unit const& unit = p_state->unit;
    if( unit.nation() != nation ) continue;
    // This should not yield and squares that don't exist.
    vector<Coord> const visible = unit_visible_squares(
        ss, nation, unit.type(), world->coord );
    for( Coord const coord : visible ) fogged.erase( coord );
  }

  // Unfog the surroundings of colonies.
  timer.checkpoint( "de-fog colony surroundings" );
  vector<ColonyId> const colonies =
      ss.colonies.for_nation( nation );
  for( ColonyId const colony_id : colonies ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    Coord const   coord  = colony.location;
    fogged.erase( coord );
    for( e_direction const d : refl::enum_values<e_direction> ) {
      Coord const moved = coord.moved( d );
      if( !ss.terrain.square_exists( moved ) ) continue;
      fogged.erase( moved );
    }
  }

  // Now affect the changes in batch.
  timer.checkpoint( "make_squares_fogged" );
  ts.map_updater.make_squares_fogged(
      nation, vector<Coord>( fogged.begin(), fogged.end() ) );
}

void update_map_visibility( TS&                   ts,
                            maybe<e_nation> const nation ) {
  ts.map_updater.mutate_options_and_redraw(
      [&]( MapUpdaterOptions& options ) {
        // This should trigger a redraw but only if we're
        // changing the nation.
        options.nation = nation;
      } );
  ts.planes.land_view().set_visibility( nation );
}

bool should_animate_move( Visibility const& viz, Coord src,
                          Coord dst ) {
  return ( viz.visible( src ) ==
           e_tile_visibility::visible_and_clear ) ||
         ( viz.visible( dst ) ==
           e_tile_visibility::visible_and_clear );
}

} // namespace rn
