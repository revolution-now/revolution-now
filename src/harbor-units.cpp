/****************************************************************
**harbor-units.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-07-08.
*
* Description:
*
*****************************************************************/
#include "harbor-units.hpp"

// Revolution Now
#include "error.hpp"
#include "logger.hpp"
#include "on-map.hpp"
#include "ustate.hpp"
#include "variant.hpp"

// config
#include "config/unit-type.rds.hpp"

// gs
#include "ss/player.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// config
#include "config/harbor.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"

// base-util
#include "base-util/algo.hpp"

using namespace std;

namespace rn {

namespace {

// All of the docs on the original game state that travel time
// from the west map edge is longer than for the east map edge,
// and furthermore than when you get Magellan that west-edge
// travel time is reduced. However, testing on the original game
// (at least version 3.0) suggests that this is not the case.
// West edge travel time is the same as east edge travel time,
// which is to say two turns (and it remains two turns even after
// the time scale change in 1600).
//
// But it does seem sensible to have the west edge travel time be
// longer (and to therefore make Magellan more attractive), so we
// will allow for that possibility here.
int turns_needed_for_high_seas(
    TerrainState const& terrain_state, Player const& player,
    UnitHarborViewState const& harbor_state ) {
  // First find where the unit sailed from (east or west). If
  // that information is not available then just assume east
  // edge.
  Coord sailed_from = harbor_state.sailed_from.value_or(
      terrain_state.world_rect_tiles().lower_right() );

  if( sailed_from.x >=
      terrain_state.world_rect_tiles().right_edge() / 2 )
    return config_harbor.high_seas_turns_east;

  // We're on the west.
  return player.fathers
                 .has[e_founding_father::ferdinand_magellan]
             ? config_harbor.high_seas_turns_west_post_magellan
             : config_harbor.high_seas_turns_west;
}

vector<UnitId> units_in_harbor_view(
    UnitsState const& units_state, e_nation nation ) {
  vector<UnitId> res;
  for( auto const& [id, st] : units_state.all() ) {
    if( st.unit.nation() == nation &&
        st.ownership.holds<UnitOwnership::harbor>() )
      res.push_back( id );
  }
  return res;
}

template<typename Func>
vector<UnitId> units_in_harbor_filtered(
    UnitsState const& units_state, e_nation nation,
    Func&& func ) {
  vector<UnitId> res =
      units_in_harbor_view( units_state, nation );
  erase_if( res, not_fn( [&]( UnitId id ) {
              return func( units_state, id );
            } ) );
  return res;
}

// Find the place where we are supposed to put the unit when it
// arrives from the new world. This is just a candidate because
// there are some further checks that need to be done on the
// square to make sure it is valid. The vast majority of the time
// it will be valid though.
Coord find_new_world_arrival_square_candidate(
    Player const& player, UnitHarborViewState const& info ) {
  if( info.sailed_from.has_value() )
    // The unit sailed from the new world, so the square from
    // which it came will have been recorded.
    return *info.sailed_from;

  if( player.last_high_seas.has_value() )
    // Fall back to the source square from which the last ship
    // moved that sailed the high seas.
    return *player.last_high_seas;

  // Finally, fall back to the original ship starting position
  // for this player.
  return player.starting_position;
}

bool is_unit_on_dock( UnitsState const& units_state,
                      UnitId            id ) {
  auto harbor_status =
      units_state.maybe_harbor_view_state_of( id );
  return harbor_status.has_value() &&
         !units_state.unit_for( id ).desc().ship &&
         holds<PortStatus::in_port>(
             harbor_status->port_status );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void update_harbor_selected_unit( UnitsState const& units,
                                  Player&           player ) {
  maybe<UnitId>& selected_unit =
      player.old_world.harbor_state.selected_unit;
  if( selected_unit.has_value() ) {
    if( !units.exists( *selected_unit ) ||
        !units.ownership_of( *selected_unit )
             .holds<UnitOwnership::harbor>() )
      // This can happen just after a ship that was selected
      // moves to the new world and/or gets disbanded.
      selected_unit.reset();
  }
  vector<UnitId> ships =
      harbor_units_in_port( units, player.nation );
  if( !ships.empty() ) selected_unit = ships[0];
}

// Find the right place to put a ship which has just arrived from
// europe.
maybe<Coord> find_new_world_arrival_square(
    UnitsState const&    units_state,
    ColoniesState const& colonies_state,
    TerrainState const& terrain_state, Player const& player,
    UnitHarborViewState const& info ) {
  Coord candidate =
      find_new_world_arrival_square_candidate( player, info );

  maybe<e_nation> nation = nation_from_coord(
      units_state, colonies_state, candidate );

  if( !nation.has_value() ) return candidate;

  // We have a case where there are units on the candidate
  // square, so let's make sure they are friendly.
  if( nation == player.nation ) return candidate;

  // The units on the square are not friendly, so we cannot drop
  // the unit here. We will procede to search the squares in an
  // outward fashion until we find one.
  Delta const world_size = terrain_state.world_size_tiles();
  Rect search = Rect::from( candidate, Delta{ .w = 1, .h = 1 } );
  int  max_radius = std::max( world_size.w, world_size.h );

  for( int radius = 0; radius < max_radius; ++radius ) {
    search = search.with_border_added();
    for( Coord c : search ) {
      maybe<MapSquare const&> square =
          terrain_state.maybe_square_at( c );
      if( !square.has_value() ) continue;
      if( square->surface != e_surface::water ) continue;
      maybe<e_nation> nation =
          nation_from_coord( units_state, colonies_state, c );
      if( !nation.has_value() || nation == player.nation )
        // We've found a square that is water and does not con-
        // tain a foreign nation.
        return c;
    }
  }

  // Theoretically at this point we should also make sure that we
  // haven't placed the ship into a lake in which it will be
  // trapped... but that seems like a rare occurrence.
  return nothing;
}

bool is_unit_inbound( UnitsState const& units_state,
                      UnitId            id ) {
  auto harbor_status =
      units_state.maybe_harbor_view_state_of( id );
  auto is_inbound =
      harbor_status.has_value() &&
      holds<PortStatus::inbound>( harbor_status->port_status );
  if( is_inbound ) {
    CHECK( units_state.unit_for( id ).desc().ship );
  }
  return is_inbound;
}

bool is_unit_outbound( UnitsState const& units_state,
                       UnitId            id ) {
  auto harbor_status =
      units_state.maybe_harbor_view_state_of( id );
  auto is_outbound =
      harbor_status.has_value() &&
      holds<PortStatus::outbound>( harbor_status->port_status );
  if( is_outbound ) {
    CHECK( units_state.unit_for( id ).desc().ship );
  }
  return is_outbound;
}

bool is_unit_in_port( UnitsState const& units_state,
                      UnitId            id ) {
  auto harbor_status =
      units_state.maybe_harbor_view_state_of( id );
  return harbor_status.has_value() &&
         units_state.unit_for( id ).desc().ship &&
         holds<PortStatus::in_port>(
             harbor_status->port_status );
}

vector<UnitId> harbor_units_on_dock(
    UnitsState const& units_state, e_nation nation ) {
  vector<UnitId> res = units_in_harbor_filtered(
      units_state, nation, is_unit_on_dock );
  // Now we must order the units by their arrival time in port
  // (or on dock).
  sort( res.begin(), res.end() );
  return res;
}

vector<UnitId> harbor_units_in_port(
    UnitsState const& units_state, e_nation nation ) {
  vector<UnitId> res = units_in_harbor_filtered(
      units_state, nation, is_unit_in_port );
  // Now we must order the units by their arrival time in port
  // (or on dock).
  sort( res.begin(), res.end() );
  return res;
}

// To old world.
vector<UnitId> harbor_units_inbound(
    UnitsState const& units_state, e_nation nation ) {
  return units_in_harbor_filtered( units_state, nation,
                                   is_unit_inbound );
}

// To new world.
vector<UnitId> harbor_units_outbound(
    UnitsState const& units_state, e_nation nation ) {
  return units_in_harbor_filtered( units_state, nation,
                                   is_unit_outbound );
}

void unit_move_to_port( UnitsState& units_state, Player& player,
                        UnitId id ) {
  UnitHarborViewState new_state;
  if( maybe<UnitHarborViewState const&> existing_state =
          units_state.maybe_harbor_view_state_of( id );
      existing_state.has_value() ) {
    new_state             = *existing_state;
    new_state.port_status = PortStatus::in_port{};
  } else {
    new_state = { .port_status = PortStatus::in_port{},
                  .sailed_from = nothing };
  }
  units_state.change_to_harbor_view( id, new_state );
  update_harbor_selected_unit( units_state, player );
}

void unit_sail_to_harbor( TerrainState const& terrain_state,
                          UnitsState&         units_state,
                          Player& player, UnitId id ) {
  // FIXME: do other checks here, e.g., make sure that the
  //        ship is not damaged.
  CHECK( units_state.unit_for( id ).desc().ship );

  if( maybe<UnitHarborViewState const&> previous_harbor_state =
          units_state.maybe_harbor_view_state_of( id );
      previous_harbor_state.has_value() ) {
    int const turns_needed = turns_needed_for_high_seas(
        terrain_state, player, *previous_harbor_state );
    switch( auto& v = previous_harbor_state->port_status;
            v.to_enum() ) {
      case PortStatus::e::in_port: return;
      case PortStatus::e::inbound: {
        auto const& [turns] = v.get<PortStatus::inbound>();
        if( turns >= turns_needed )
          // Unit has not yet made any progress, so we can imme-
          // diately move it to in_port.
          unit_move_to_port( units_state, player, id );
        return;
      }
      case PortStatus::e::outbound: {
        auto const& [turns] = v.get<PortStatus::outbound>();
        UnitHarborViewState new_state = *previous_harbor_state;
        // Unit must "turn around" and go the other way.
        new_state.port_status =
            PortStatus::inbound{ .turns = turns_needed - turns };
        units_state.change_to_harbor_view( id, new_state );
        // Recurse to deal with the inbound state, which might in
        // turn need to be translated to in_port.
        unit_sail_to_harbor( terrain_state, units_state, player,
                             id );
        return;
      }
    }
  }

  maybe<Coord> sailed_from = units_state.maybe_coord_for( id );
  // Even though last_high_seas is a maybe<Coord>, don't over-
  // write it unless the new position has a value.
  if( sailed_from.has_value() )
    player.last_high_seas = sailed_from;

  // Unit is not owned by the harbor view, so let's make it so.
  // If the unit is currently on the map then record that posi-
  // tion so that it can return to it.
  units_state.change_to_harbor_view(
      id, UnitHarborViewState{
              .port_status = PortStatus::inbound{ .turns = 0 },
              .sailed_from = sailed_from } );
}

void unit_sail_to_new_world( TerrainState const& terrain_state,
                             UnitsState&         units_state,
                             Player const& player, UnitId id ) {
  // FIXME: do other checks here, e.g., make sure that the
  //        ship is not damaged.
  CHECK( units_state.unit_for( id ).desc().ship );

  UNWRAP_CHECK( previous_harbor_state,
                units_state.maybe_harbor_view_state_of( id ) );

  // Note that we are always reusing the `sailed_from`.
  UnitHarborViewState new_state = previous_harbor_state;

  switch( auto& v = previous_harbor_state.port_status;
          v.to_enum() ) {
    case PortStatus::e::outbound: //
      // Even if the progress is complete, we don't move the unit
      // onto the map, since that is not the job of this func-
      // tion.
      return;
    case PortStatus::e::inbound: {
      auto& [turns]          = v.get<PortStatus::inbound>();
      int const turns_needed = turns_needed_for_high_seas(
          terrain_state, player, previous_harbor_state );
      // Unit must "turn around" and go the other way.
      new_state.port_status =
          PortStatus::outbound{ .turns = turns_needed - turns };
      break;
    }
    case PortStatus::e::in_port: {
      new_state.port_status = PortStatus::outbound{ .turns = 0 };
      break;
    }
  }

  units_state.change_to_harbor_view( id, new_state );
}

e_high_seas_result advance_unit_on_high_seas(
    TerrainState const& terrain_state, UnitsState& units_state,
    Player& player, UnitId id ) {
  UNWRAP_CHECK( info,
                units_state.maybe_harbor_view_state_of( id ) );
  int const turns_needed =
      turns_needed_for_high_seas( terrain_state, player, info );

  if_get( info.port_status, PortStatus::outbound, outbound ) {
    ++outbound.turns;
    outbound.turns =
        std::clamp( outbound.turns, 0, turns_needed );
    lg.debug( "advancing outbound unit {} to {} turns.",
              debug_string( units_state, id ), outbound.turns );
    if( outbound.turns >= turns_needed )
      return e_high_seas_result::arrived_in_new_world;
    return e_high_seas_result::still_traveling;
  }

  if_get( info.port_status, PortStatus::inbound, inbound ) {
    ++inbound.turns;
    inbound.turns = std::clamp( inbound.turns, 0, turns_needed );
    lg.debug( "advancing inbound unit {} to {} turns.",
              debug_string( units_state, id ), inbound.turns );
    if( inbound.turns >= turns_needed ) {
      // This should preserve the `sailed_from`.
      unit_move_to_port( units_state, player, id );
      return e_high_seas_result::arrived_in_harbor;
    }
    return e_high_seas_result::still_traveling;
  }

  FATAL( "{} is not on the high seas.",
         debug_string( units_state, id ) );
}

UnitId create_unit_in_harbor( UnitsState&     units_state,
                              Player&         player,
                              UnitComposition comp ) {
  UnitId id =
      create_free_unit( units_state, player, std::move( comp ) );
  unit_move_to_port( units_state, player, id );
  return id;
}

UnitId create_unit_in_harbor( UnitsState& units_state,
                              Player&     player,
                              e_unit_type type ) {
  return create_unit_in_harbor(
      units_state, player, UnitComposition::create( type ) );
}

} // namespace rn
