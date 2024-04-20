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
#include "ss/natives.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/iter.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/timer.hpp"

using namespace std;

namespace rn {

namespace {

using unexplored = PlayerSquare::unexplored;
using explored   = PlayerSquare::explored;
using fogged     = FogStatus::fogged;
using clear      = FogStatus::clear;

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
** IVisibility
*****************************************************************/
IVisibility::IVisibility( SSConst const& ss )
  : terrain_( ss.terrain ) {}

Rect IVisibility::rect_tiles() const {
  return terrain_.world_rect_tiles();
}

bool IVisibility::on_map( Coord tile ) const {
  return tile.is_inside( rect_tiles() );
}

maybe<e_natural_resource> IVisibility::resource_at(
    Coord tile ) const {
  maybe<e_natural_resource> const res =
      effective_resource( square_at( tile ) );
  // Check this first for optimization purposes.
  if( !res.has_value() ) return nothing;
  if( is_resource_suppressed( tile ) ) return nothing;
  CHECK( res.has_value() );
  return *res;
}

bool IVisibility::is_resource_suppressed( Coord tile ) const {
  MapSquare const& square = square_at( tile );
  if( square.lost_city_rumor ) return true;
  // The OG suppresses the rendering of prime resources under na-
  // tive dwellings, probably in order to create a tradeoff for
  // the player where they have to burn the dwelling to see what
  // is under it. However, it does seem to render them under
  // colonies (both friendly and foreign).
  if( dwelling_at( tile ).has_value() ) return true;
  return false;
}

/****************************************************************
** VisibilityEntire
*****************************************************************/
VisibilityEntire::VisibilityEntire( SSConst const& ss )
  : IVisibility( ss ), ss_( ss ) {}

e_tile_visibility VisibilityEntire::visible( Coord ) const {
  return e_tile_visibility::clear;
}

MapSquare const& VisibilityEntire::square_at(
    Coord tile ) const {
  return ss_.terrain.total_square_at( tile );
};

maybe<Colony const&> VisibilityEntire::colony_at(
    Coord tile ) const {
  return ss_.colonies.maybe_from_coord( tile ).fmap(
      [&]( ColonyId const colony_id ) -> Colony const& {
        return ss_.colonies.colony_for( colony_id );
      } );
}

maybe<Dwelling const&> VisibilityEntire::dwelling_at(
    Coord tile ) const {
  return ss_.natives.maybe_dwelling_from_coord( tile ).fmap(
      [&]( DwellingId const dwelling_id ) -> Dwelling const& {
        return ss_.natives.dwelling_for( dwelling_id );
      } );
}

/****************************************************************
** VisibilityForNation
*****************************************************************/
VisibilityForNation::VisibilityForNation( SSConst const& ss,
                                          e_nation       nation )
  : IVisibility( ss ),
    ss_( ss ),
    entire_( ss ),
    nation_( nation ),
    player_terrain_( addressof(
        ss.terrain.player_terrain( nation ).value() ) ) {}

e_tile_visibility VisibilityForNation::visible(
    Coord tile ) const {
  maybe<PlayerSquare const&> player_square =
      player_square_at( tile );
  if( !player_square.has_value() )
    // Proto square.
    return e_tile_visibility::hidden;
  SWITCH( *player_square ) {
    CASE( unexplored ) { return e_tile_visibility::hidden; }
    CASE( explored ) {
      SWITCH( explored.fog_status ) {
        CASE( fogged ) { return e_tile_visibility::fogged; }
        CASE( clear ) { return e_tile_visibility::clear; }
      }
    }
  }
}

maybe<PlayerSquare const&> VisibilityForNation::player_square_at(
    Coord tile ) const {
  // If it's a proto square then can't obtain a player square.
  if( !on_map( tile ) ) return nothing;
  return player_terrain_->map[tile];
}

maybe<Colony const&> VisibilityForNation::colony_at(
    Coord tile ) const {
  UNWRAP_RETURN( player_square, player_square_at( tile ) );
  SWITCH( player_square ) {
    CASE( unexplored ) { return nothing; }
    CASE( explored ) {
      SWITCH( explored.fog_status ) {
        CASE( fogged ) { return fogged.contents.colony; }
        CASE( clear ) { return entire_.colony_at( tile ); }
      }
    }
  }
}

maybe<Dwelling const&> VisibilityForNation::dwelling_at(
    Coord tile ) const {
  UNWRAP_RETURN( player_square, player_square_at( tile ) );
  SWITCH( player_square ) {
    CASE( unexplored ) { return nothing; }
    CASE( explored ) {
      SWITCH( explored.fog_status ) {
        CASE( fogged ) { return fogged.contents.dwelling; }
        CASE( clear ) { return entire_.dwelling_at( tile ); }
      }
    }
  }
}

MapSquare const& VisibilityForNation::square_at(
    Coord tile ) const {
  maybe<PlayerSquare const&> const player_square =
      player_square_at( tile );
  if( !player_square.has_value() )
    // Proto square.
    return entire_.square_at( tile );
  SWITCH( *player_square ) {
    CASE( unexplored ) {
      // NOTE: There is an interesting issue here. When a square
      // is unexplored for the player then they will always get
      // the real tile. That may seem unimportant, but it does
      // because the tile returned will affect how surrounding
      // tiles are rendered (which the player may have some visi-
      // bility into). Thus the player can see changes to the un-
      // explored tile even if its surrounding tiles are fogged,
      // which looks wrong, especially since they player does not
      // receive updates on fogged tiles. So e.g. if an AI player
      // clears a forest on a tile that is unexplored but has
      // fogged surroundings, the effects will still be seen to
      // the player via those fogged surroundings. One can also
      // reproduce this using the map editor. A solution was at-
      // tempted for this (saved on a branch); it basically
      // worked, though it introduced some complexity and new in-
      // consistencies, so it was abandoned. Current thinking is
      // that this is not necessary (or worth it) to fix.
      return entire_.square_at( tile );
    }
    CASE( explored ) {
      SWITCH( explored.fog_status ) {
        CASE( fogged ) { return fogged.contents.square; }
        CASE( clear ) { return entire_.square_at( tile ); }
      }
    }
  }
}

/****************************************************************
** VisibilityWithOverrides
*****************************************************************/
VisibilityWithOverrides::VisibilityWithOverrides(
    SSConst const& ss, IVisibility const& underlying,
    VisibilityOverrides const& overrides )
  : IVisibility( ss ),
    underlying_( underlying ),
    overrides_( overrides ) {}

e_tile_visibility VisibilityWithOverrides::visible(
    Coord tile ) const {
  return underlying_.visible( tile );
}

maybe<Colony const&> VisibilityWithOverrides::colony_at(
    Coord tile ) const {
  return underlying_.colony_at( tile );
}

maybe<Dwelling const&> VisibilityWithOverrides::dwelling_at(
    Coord tile ) const {
  if( auto it = overrides_.dwellings.find( tile );
      it != overrides_.dwellings.end() )
    return it->second;
  return underlying_.dwelling_at( tile );
}

MapSquare const& VisibilityWithOverrides::square_at(
    Coord tile ) const {
  if( auto it = overrides_.squares.find( tile );
      it != overrides_.squares.end() )
    return it->second;
  return underlying_.square_at( tile );
}

/****************************************************************
** Public API
*****************************************************************/
std::unique_ptr<IVisibility const> create_visibility_for(
    SSConst const& ss, maybe<e_nation> nation ) {
  if( nation.has_value() )
    return make_unique<VisibilityForNation const>( ss, *nation );
  else
    return make_unique<VisibilityEntire const>( ss );
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
  PlayerSquare const& player_square = player_terrain.map[tile];
  return player_square.inner_if<explored>()
      .get_if<clear>()
      .has_value();
}

void recompute_fog_for_nation( SS& ss, TS& ts,
                               e_nation nation ) {
  base::ScopedTimer timer(
      fmt::format( "regenerating fog-of-war ({})", nation ) );
  timer.options().no_checkpoints_logging = true;

  UNWRAP_CHECK( player_terrain,
                ss.terrain.player_terrain( nation ) );
  auto const& m = player_terrain.map;

  timer.checkpoint( "generate fogged set" );
  unordered_set<Coord> fogged;
  for( int y = 0; y < m.size().h; ++y ) {
    for( int x = 0; x < m.size().w; ++x ) {
      Coord const coord{ .x = x, .y = y };
      SWITCH( m[coord] ) {
        CASE( unexplored ) { continue; }
        CASE( explored ) {
          SWITCH( explored.fog_status ) {
            CASE( fogged ) { continue; }
            CASE( clear ) {
              fogged.insert( coord );
              continue;
            }
          }
        }
      }
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
    for( Coord const coord : visible ) { fogged.erase( coord ); }
  }

  // Unfog the surroundings of colonies.
  //
  // Note regarding de soto: in the OG, the sighting radius of a
  // colony is always 1 (with or without de soto) unless there is
  // at least one unit (of any kind) at the gate. In the latter
  // case, the sighting radius is determined by the unit's
  // sighting radius given the de soto status, as well as the
  // sighting limitations (i.e., land units can't see into
  // oceans, etc.). All that we have to do to replicate that is
  // to use a fixed sighting radius of 1 below for the colonies,
  // since if there are any units at the gate then they would
  // have already been accounted for in the units section above,
  // and the right thing will have been done.
  timer.checkpoint( "de-fog colony surroundings" );
  vector<ColonyId> const colonies =
      ss.colonies.for_nation( nation );
  for( ColonyId const colony_id : colonies ) {
    Colony const& colony = ss.colonies.colony_for( colony_id );
    Coord const   coord  = colony.location;
    fogged.erase( coord );
    for( e_direction const d1 :
         refl::enum_values<e_direction> ) {
      fogged.erase( coord.moved( d1 ) );
    }
  }

  // Now affect the changes in batch.
  timer.checkpoint( "make_squares_fogged" );
  ts.map_updater().make_squares_fogged(
      nation, vector<Coord>( fogged.begin(), fogged.end() ) );
}

void update_map_visibility( TS&                   ts,
                            maybe<e_nation> const nation ) {
  ts.map_updater().mutate_options_and_redraw(
      [&]( MapUpdaterOptions& options ) {
        // This should trigger a redraw but only if we're
        // changing the nation.
        options.nation = nation;
      } );
  ts.planes.get().get_bottom<ILandViewPlane>().set_visibility(
      nation );
}

bool should_animate_move( IVisibility const& viz, Coord src,
                          Coord dst ) {
  return ( viz.visible( src ) == e_tile_visibility::clear ) ||
         ( viz.visible( dst ) == e_tile_visibility::clear );
}

} // namespace rn
