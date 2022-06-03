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
#include "game-state.hpp"
#include "gs-units.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "on-map.hpp"
#include "player.hpp"
#include "ustate.hpp"
#include "variant.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/lambda.hpp"

// base-util
#include "base-util/algo.hpp"

using namespace std;

namespace rn {

namespace {

template<typename Func>
vector<UnitId> units_in_harbor_filtered(
    UnitsState const& units_state, Func&& func ) {
  vector<UnitId> res = units_in_harbor_view();
  erase_if( res, not_fn( [&]( UnitId id ) {
              return func( units_state, id );
            } ) );
  return res;
}

// Find the right place to put a ship which has just arrived from
// europe.
Coord find_arrival_square( Player const&              player,
                           UnitHarborViewState const& info ) {
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
         !unit_from_id( id ).desc().ship &&
         holds<PortStatus::in_port>(
             harbor_status->port_status );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
bool is_unit_inbound( UnitsState const& units_state,
                      UnitId            id ) {
  auto harbor_status =
      units_state.maybe_harbor_view_state_of( id );
  auto is_inbound =
      harbor_status.has_value() &&
      holds<PortStatus::inbound>( harbor_status->port_status );
  if( is_inbound ) { CHECK( unit_from_id( id ).desc().ship ); }
  return is_inbound;
}

bool is_unit_outbound( UnitsState const& units_state,
                       UnitId            id ) {
  auto harbor_status =
      units_state.maybe_harbor_view_state_of( id );
  auto is_outbound =
      harbor_status.has_value() &&
      holds<PortStatus::outbound>( harbor_status->port_status );
  if( is_outbound ) { CHECK( unit_from_id( id ).desc().ship ); }
  return is_outbound;
}

bool is_unit_in_port( UnitsState const& units_state,
                      UnitId            id ) {
  auto harbor_status =
      units_state.maybe_harbor_view_state_of( id );
  return harbor_status.has_value() &&
         unit_from_id( id ).desc().ship &&
         holds<PortStatus::in_port>(
             harbor_status->port_status );
}

vector<UnitId> harbor_units_on_dock(
    UnitsState const& units_state ) {
  vector<UnitId> res =
      units_in_harbor_filtered( units_state, is_unit_on_dock );
  // Now we must order the units by their arrival time in port
  // (or on dock).
  sort( res.begin(), res.end() );
  return res;
}

vector<UnitId> harbor_units_in_port(
    UnitsState const& units_state ) {
  vector<UnitId> res =
      units_in_harbor_filtered( units_state, is_unit_in_port );
  // Now we must order the units by their arrival time in port
  // (or on dock).
  sort( res.begin(), res.end() );
  return res;
}

// To old world.
vector<UnitId> harbor_units_inbound(
    UnitsState const& units_state ) {
  return units_in_harbor_filtered( units_state,
                                   is_unit_inbound );
}

// To new world.
vector<UnitId> harbor_units_outbound(
    UnitsState const& units_state ) {
  return units_in_harbor_filtered( units_state,
                                   is_unit_outbound );
}

void unit_move_to_port( UnitsState& units_state, UnitId id ) {
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
}

void unit_sail_to_harbor( UnitsState& units_state,
                          Player& player, UnitId id ) {
  // FIXME: do other checks here, e.g., make sure that the
  //        ship is not damaged.
  CHECK( units_state.unit_for( id ).desc().ship );

  if( maybe<UnitHarborViewState const&> existing_state =
          units_state.maybe_harbor_view_state_of( id );
      existing_state.has_value() ) {
    switch( auto& v = existing_state->port_status;
            v.to_enum() ) {
      case PortStatus::e::in_port: return;
      case PortStatus::e::inbound: return;
      case PortStatus::e::outbound: {
        auto const& [percent] = v.get<PortStatus::outbound>();
        UnitHarborViewState new_state = *existing_state;
        if( percent > 0.0 ) {
          // Unit must "turn around" and go the other way.
          new_state.port_status = PortStatus::inbound{
              /*progress=*/( 1.0 - percent ) };
        } else {
          // Unit has not yet made any progress, so we can imme-
          // diately move it to in_port.
          new_state.port_status = PortStatus::in_port{};
        }
        units_state.change_to_harbor_view( id, new_state );
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
      id,
      UnitHarborViewState{
          .port_status = PortStatus::inbound{ /*progress=*/0.0 },
          .sailed_from = sailed_from } );
}

void unit_sail_to_new_world( UnitsState& units_state,
                             UnitId      id ) {
  // FIXME: do other checks here, e.g., make sure that the
  //        ship is not damaged.
  CHECK( units_state.unit_for( id ).desc().ship );

  UNWRAP_CHECK( existing_state,
                units_state.maybe_harbor_view_state_of( id ) );

  // Note that we are always reusing the `sailed_from`.
  UnitHarborViewState new_state = existing_state;

  switch( auto& v = existing_state.port_status; v.to_enum() ) {
    case PortStatus::e::outbound: //
      return;
    case PortStatus::e::inbound: {
      auto& [percent] = v.get<PortStatus::inbound>();
      // Unit must "turn around" and go the other way.
      new_state.port_status =
          PortStatus::outbound{ /*progress=*/( 1.0 - percent ) };
      break;
    }
    case PortStatus::e::in_port: {
      new_state.port_status =
          PortStatus::outbound{ /*progress=*/0.0 };
      break;
    }
  }

  units_state.change_to_harbor_view( id, new_state );
}

e_high_seas_result advance_unit_on_high_seas(
    UnitsState& units_state, Player const& player, UnitId id,
    IMapUpdater& map_updater ) {
  UNWRAP_CHECK( info,
                units_state.maybe_harbor_view_state_of( id ) );
  constexpr double const advance = 0.25;

  if_get( info.port_status, PortStatus::outbound, outbound ) {
    outbound.percent += advance;
    outbound.percent = std::clamp( outbound.percent, 0.0, 1.0 );
    lg.debug( "advancing outbound unit {} to {} percent.",
              debug_string( id ), outbound.percent );
    if( outbound.percent >= 1.0 ) {
      // FIXME: temporary; also, would want to use coroutine ver-
      // sion of this function.
      unit_to_map_square_no_ui(
          units_state, map_updater, id,
          find_arrival_square( player, info ) );
      units_state.unit_for( id ).clear_orders();
      lg.debug( "unit has arrived in new world." );
      return e_high_seas_result::arrived_in_new_world;
    }
    return e_high_seas_result::still_traveling;
  }

  if_get( info.port_status, PortStatus::inbound, inbound ) {
    inbound.percent += advance;
    inbound.percent = std::clamp( inbound.percent, 0.0, 1.0 );
    lg.debug( "advancing inbound unit {} to {} percent.",
              debug_string( id ), inbound.percent );
    if( inbound.percent >= 1.0 ) {
      // This should preserve the `sailed_from`.
      unit_move_to_port( units_state, id );
      lg.debug( "unit has arrived in old world." );
      return e_high_seas_result::arrived_in_harbor;
    }
    return e_high_seas_result::still_traveling;
  }

  FATAL( "{} is not on the high seas.", debug_string( id ) );
}

UnitId create_unit_in_harbor( UnitsState&     units_state,
                              e_nation        nation,
                              UnitComposition comp ) {
  UnitId id =
      create_unit( units_state, nation, std::move( comp ) );
  unit_move_to_port( units_state, id );
  return id;
}

UnitId create_unit_in_harbor( UnitsState& units_state,
                              e_nation    nation,
                              e_unit_type type ) {
  return create_unit_in_harbor(
      units_state, nation, UnitComposition::create( type ) );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( create_unit_in_port, UnitId, e_nation nation,
        UnitComposition comp ) {
  return create_unit_in_harbor( GameState::units(), nation,
                                comp );
};

} // namespace

} // namespace rn
