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
#include "connectivity.hpp"
#include "error.hpp"
#include "logger.hpp"
#include "on-map.hpp"
#include "society.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"
#include "variant.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// config
#include "config/harbor.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/generator.hpp"
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
    UnitOwnership::harbor const& harbor_state ) {
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
  for( auto const& [id, st] : units_state.euro_all() ) {
    if( st->unit.nation() == nation &&
        st->ownership.holds<UnitOwnership::harbor>() )
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

bool is_unit_on_dock( UnitsState const& units_state,
                      UnitId            id ) {
  auto harbor_status =
      units_state.maybe_harbor_view_state_of( id );
  return harbor_status.has_value() &&
         !units_state.unit_for( id ).desc().ship &&
         holds<PortStatus::in_port>(
             harbor_status->port_status );
}

base::generator<Coord> search_from_square( Coord const start,
                                           int max_radius ) {
  Coord upper_left = start;
  co_yield start;
  for( int radius = 1; radius < max_radius; ++radius ) {
    --upper_left.x;
    --upper_left.y;
    int const side_length = radius * 2 + 1;
    // Now we're going to yield the coordinates that comprise the
    // edge squares around the square of side length
    // `side_length` centered on `start` (1-9,a-p):
    //
    //   a b c d e
    //   f 2 3 4 g
    //   h 5 1 6 i
    //   j 7 8 9 k
    //   l m n o p
    //
    // First yield the top row of the square.
    Coord c = upper_left;
    for( int j = 0; j < side_length; ++j ) {
      co_yield c;
      ++c.x;
    }

    // Now yield the sides, one row at a time.
    c = upper_left;
    for( int j = 0; j < side_length - 2; ++j ) {
      ++c.y;
      Coord left  = c;
      Coord right = c;
      right.x += side_length - 1;
      co_yield left;
      co_yield right;
    }

    // Now yield bottom.
    c = upper_left;
    c.y += side_length - 1;
    for( int j = 0; j < side_length; ++j ) {
      co_yield c;
      ++c.x;
    }
  }
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
    SSConst const& ss, TS& ts, Player const& player,
    maybe<Coord> sailed_from ) {
  Coord const candidate = sailed_from.has_value() ? *sailed_from
                          : player.last_high_seas.has_value()
                              ? *player.last_high_seas
                              : player.starting_position;

  // The units on the square are not friendly, so we cannot drop
  // the unit here. We will procede to search the squares in an
  // outward fashion until we find one.
  Delta const world_size = ss.terrain.world_size_tiles();
  int max_radius = std::max( world_size.w, world_size.h );

  for( Coord c : search_from_square( candidate, max_radius ) ) {
    maybe<MapSquare const&> square =
        ss.terrain.maybe_square_at( c );
    if( !square.has_value() ) continue;
    if( square->surface != e_surface::water ) continue;
    // We've found a square that is water Now just make sure that
    // it is not part of an inland lake. Ships are not alowed in
    // there to begin with, and even if they were, it would prob-
    // ably be frustrating to the player because they may not
    // then be able to get the ship out.
    if( is_inland_lake( ts.connectivity, c ) ) continue;
    maybe<Society> society = society_on_square( ss, c );
    if( !society.has_value() ||
        society == Society{ Society::european{
                       .nation = player.nation } } )
      return c;
  }

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

void unit_move_to_port( SS& ss, UnitId id ) {
  Unit&   unit = ss.units.unit_for( id );
  Player& player =
      player_for_nation_or_die( ss.players, unit.nation() );
  UnitOwnership::harbor new_state;
  if( maybe<UnitOwnership::harbor const&> existing_state =
          ss.units.maybe_harbor_view_state_of( id );
      existing_state.has_value() ) {
    new_state             = *existing_state;
    new_state.port_status = PortStatus::in_port{};
  } else {
    new_state = { .port_status = PortStatus::in_port{},
                  .sailed_from = nothing };
  }
  UnitOwnershipChanger( ss, id ).change_to_harbor(
      new_state.port_status, new_state.sailed_from );
  if( !unit.orders().holds<unit_orders::damaged>() &&
      unit.desc().ship )
    // There are various scenarios in the game where a ship that
    // is fortified or sentried can get immediatelly transported
    // back to the harbor even if it is not damaged. In those
    // cases we want to make sure that its orders are cleared,
    // because the harbor UI does not provide a way to clear
    // them, and so if they are not cleared then they will never
    // be moved. For units, on the other hand, they typically
    // will already have cleared orders when on the dock, but it
    // is not necessary that they be clear, since they are moved
    // by dragging them to a ship. If the orders are "damaged"
    // then we don't have to do this, since that is a normal game
    // mechanic.
    unit.clear_orders();
  update_harbor_selected_unit( ss.units, player );
}

void unit_sail_to_harbor( SS& ss, UnitId id ) {
  // FIXME: do other checks here, e.g., make sure that the
  //        ship is not damaged.
  Unit& unit = ss.units.unit_for( id );
  CHECK( unit.desc().ship );
  CHECK( unit.orders().holds<unit_orders::none>() );
  Player& player =
      player_for_nation_or_die( ss.players, unit.nation() );

  if( maybe<UnitOwnership::harbor const&> previous_harbor_state =
          ss.units.maybe_harbor_view_state_of( id );
      previous_harbor_state.has_value() ) {
    int const turns_needed = turns_needed_for_high_seas(
        ss.terrain, player, *previous_harbor_state );
    switch( auto& v = previous_harbor_state->port_status;
            v.to_enum() ) {
      case PortStatus::e::in_port:
        return;
      case PortStatus::e::inbound: {
        auto const& [turns] = v.get<PortStatus::inbound>();
        if( turns >= turns_needed )
          // Unit has not yet made any progress, so we can
          // imme- diately move it to in_port.
          unit_move_to_port( ss, id );
        return;
      }
      case PortStatus::e::outbound: {
        auto const& [turns] = v.get<PortStatus::outbound>();
        UnitOwnership::harbor new_state = *previous_harbor_state;
        // Unit must "turn around" and go the other way.
        new_state.port_status =
            PortStatus::inbound{ .turns = turns_needed - turns };
        UnitOwnershipChanger( ss, id ).change_to_harbor(
            new_state.port_status, new_state.sailed_from );
        // Recurse to deal with the inbound state, which might
        // in turn need to be translated to in_port.
        unit_sail_to_harbor( ss, id );
        return;
      }
    }
  }

  maybe<Coord> sailed_from = ss.units.maybe_coord_for( id );
  // Even though last_high_seas is a maybe<Coord>, don't over-
  // write it unless the new position has a value.
  if( sailed_from.has_value() )
    player.last_high_seas = sailed_from;

  // Unit is not owned by the harbor view, so let's make it so.
  // If the unit is currently on the map then record that posi-
  // tion so that it can return to it.
  UnitOwnershipChanger( ss, id ).change_to_harbor(
      PortStatus::inbound{ .turns = 0 }, sailed_from );
}

void unit_sail_to_new_world( SS& ss, UnitId id ) {
  // FIXME: do other checks here, e.g., make sure that the
  //        ship is not damaged.
  Unit const& unit = ss.units.unit_for( id );
  CHECK( unit.desc().ship );
  CHECK( unit.orders().holds<unit_orders::none>() );
  Player const& player =
      player_for_nation_or_die( ss.players, unit.nation() );
  CHECK( unit.desc().ship );

  UNWRAP_CHECK( previous_harbor_state,
                ss.units.maybe_harbor_view_state_of( id ) );

  // Note that we are always reusing the `sailed_from`.
  UnitOwnership::harbor new_state = previous_harbor_state;

  switch( auto& v = previous_harbor_state.port_status;
          v.to_enum() ) {
    case PortStatus::e::outbound: //
      // Even if the progress is complete, we don't move the
      // unit onto the map, since that is not the job of this
      // func- tion.
      return;
    case PortStatus::e::inbound: {
      auto& [turns]          = v.get<PortStatus::inbound>();
      int const turns_needed = turns_needed_for_high_seas(
          ss.terrain, player, previous_harbor_state );
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

  UnitOwnershipChanger( ss, id ).change_to_harbor(
      new_state.port_status, new_state.sailed_from );
}

e_high_seas_result advance_unit_on_high_seas( SS&     ss,
                                              Player& player,
                                              UnitId  id ) {
  CHECK( ss.units.unit_for( id )
             .orders()
             .holds<unit_orders::none>() );
  UNWRAP_CHECK( info,
                ss.units.maybe_harbor_view_state_of( id ) );
  int const turns_needed =
      turns_needed_for_high_seas( ss.terrain, player, info );

  if_get( info.port_status, PortStatus::outbound, outbound ) {
    ++outbound.turns;
    outbound.turns =
        std::clamp( outbound.turns, 0, turns_needed );
    lg.debug( "advancing outbound unit {} to {} turns.",
              debug_string( ss.units, id ), outbound.turns );
    if( outbound.turns >= turns_needed )
      return e_high_seas_result::arrived_in_new_world;
    return e_high_seas_result::still_traveling;
  }

  if_get( info.port_status, PortStatus::inbound, inbound ) {
    ++inbound.turns;
    inbound.turns = std::clamp( inbound.turns, 0, turns_needed );
    lg.debug( "advancing inbound unit {} to {} turns.",
              debug_string( ss.units, id ), inbound.turns );
    if( inbound.turns >= turns_needed ) {
      // This should preserve the `sailed_from`.
      unit_move_to_port( ss, id );
      return e_high_seas_result::arrived_in_harbor;
    }
    return e_high_seas_result::still_traveling;
  }

  FATAL( "{} is not on the high seas.",
         debug_string( ss.units, id ) );
}

UnitId create_unit_in_harbor( SS& ss, Player& player,
                              UnitComposition comp ) {
  UnitId const id =
      create_free_unit( ss.units, player, std::move( comp ) );
  unit_move_to_port( ss, id );
  return id;
}

} // namespace rn
