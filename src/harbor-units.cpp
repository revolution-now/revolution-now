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
vector<UnitId> units_in_harbor_filtered( Func&& func ) {
  vector<UnitId> res = units_in_harbor_view();
  erase_if( res, not_fn( std::forward<Func>( func ) ) );
  return res;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
bool is_unit_on_dock( UnitId id ) {
  auto harbor_status = unit_harbor_view_info( id );
  return harbor_status.has_value() &&
         !unit_from_id( id ).desc().ship &&
         holds<UnitHarborViewState::in_port>( *harbor_status );
}

bool is_unit_inbound( UnitId id ) {
  auto harbor_status = unit_harbor_view_info( id );
  auto is_inbound =
      harbor_status.has_value() &&
      holds<UnitHarborViewState::inbound>( *harbor_status );
  if( is_inbound ) { CHECK( unit_from_id( id ).desc().ship ); }
  return is_inbound;
}

bool is_unit_outbound( UnitId id ) {
  auto harbor_status = unit_harbor_view_info( id );
  auto is_outbound =
      harbor_status.has_value() &&
      holds<UnitHarborViewState::outbound>( *harbor_status );
  if( is_outbound ) { CHECK( unit_from_id( id ).desc().ship ); }
  return is_outbound;
}

bool is_unit_in_port( UnitId id ) {
  auto harbor_status = unit_harbor_view_info( id );
  return harbor_status.has_value() &&
         unit_from_id( id ).desc().ship &&
         holds<UnitHarborViewState::in_port>( *harbor_status );
}

vector<UnitId> harbor_units_on_dock() {
  vector<UnitId> res =
      units_in_harbor_filtered( is_unit_on_dock );
  // Now we must order the units by their arrival time in port
  // (or on dock).
  sort( res.begin(), res.end() );
  return res;
}

vector<UnitId> harbor_units_in_port() {
  vector<UnitId> res =
      units_in_harbor_filtered( is_unit_in_port );
  // Now we must order the units by their arrival time in port
  // (or on dock).
  sort( res.begin(), res.end() );
  return res;
}

// To old world.
vector<UnitId> harbor_units_inbound() {
  return units_in_harbor_filtered( is_unit_inbound );
}

// To new world.
vector<UnitId> harbor_units_outbound() {
  return units_in_harbor_filtered( is_unit_outbound );
}

void unit_sail_to_harbor( UnitId id ) {
  // FIXME: do other checks here, e.g., make sure that the
  //        ship is not damaged.
  CHECK( unit_from_id( id ).desc().ship );
  // This is the state to which we will set the unit, at least by
  // default (though it might get modified below based on the
  // current state of the unit).
  UnitHarborViewState_t target_state =
      UnitHarborViewState::inbound{ /*progress=*/0.0 };
  auto maybe_state = unit_harbor_view_info( id );
  if( maybe_state ) {
    switch( auto& v = *maybe_state; v.to_enum() ) {
      case UnitHarborViewState::e::inbound: {
        auto& val = v.get<UnitHarborViewState::inbound>();
        // no-op, i.e., keep state the same.
        target_state = val;
        break;
      }
      case UnitHarborViewState::e::outbound: {
        auto& [percent] = v.get<UnitHarborViewState::outbound>();
        if( percent > 0.0 ) {
          // Unit must "turn around" and go the other way.
          target_state = UnitHarborViewState::inbound{
              /*progress=*/( 1.0 - percent ) };
        } else {
          // Unit has not yet made any progress, so we can imme-
          // diately move it to in_port.
          target_state = UnitHarborViewState::in_port{};
        }
        break;
      }
      case UnitHarborViewState::e::in_port: {
        auto& val = v.get<UnitHarborViewState::in_port>();
        // no-op, unit is already in port.
        target_state = val;
        break;
      }
    }
  }
  if( target_state == maybe_state ) //
    return;
  lg.info( "setting {} to state {}", debug_string( id ),
           target_state );
  // Note: unit may already be in a harbor state here.
  GameState::units().change_to_harbor_view( id, target_state );
}

void unit_sail_to_new_world( UnitId id ) {
  // FIXME: do other checks here, e.g., make sure that the
  //        ship is not damaged.
  CHECK( unit_from_id( id ).desc().ship );
  // This is the state to which we will set the unit, at least by
  // default (though it might get modified below based on the
  // current state of the unit).
  UnitHarborViewState_t target_state =
      UnitHarborViewState::outbound{ /*progress=*/0.0 };
  auto maybe_state = unit_harbor_view_info( id );
  CHECK( maybe_state );
  switch( auto& v = *maybe_state; v.to_enum() ) {
    case UnitHarborViewState::e::outbound: {
      auto& val = v.get<UnitHarborViewState::outbound>();
      // no-op, i.e., keep state the same.
      target_state = val;
      break;
    }
    case UnitHarborViewState::e::inbound: {
      auto& [percent] = v.get<UnitHarborViewState::inbound>();
      // Unit must "turn around" and go the other way.
      target_state = UnitHarborViewState::outbound{
          /*progress=*/( 1.0 - percent ) };
      break;
    }
    case UnitHarborViewState::e::in_port: {
      // keep default target.
      break;
    }
  }
  if( target_state == maybe_state ) //
    return;
  lg.info( "setting {} to state {}", debug_string( id ),
           target_state );
  // Note: unit may already be in a harbor state here.
  GameState::units().change_to_harbor_view( id, target_state );
}

void unit_move_to_harbor( UnitId id ) {
  if( is_unit_on_dock( id ) ) return;
  auto holder = is_unit_onboard( id );
  CHECK( holder && is_unit_in_port( *holder ),
         "cannot move unit to dock unless it is in the cargo of "
         "a ship that is in port." );
  GameState::units().change_to_harbor_view(
      id, UnitHarborViewState::in_port{} );
  DCHECK( is_unit_on_dock( id ) );
  DCHECK( !is_unit_onboard( id ) );
}

e_high_seas_result advance_unit_on_high_seas(
    UnitId id, IMapUpdater& map_updater ) {
  UNWRAP_CHECK( info, unit_harbor_view_info( id ) );
  constexpr double const advance = 0.25;
  if_get( info, UnitHarborViewState::outbound, outbound ) {
    outbound.percent += advance;
    outbound.percent = std::clamp( outbound.percent, 0.0, 1.0 );
    lg.debug( "advancing outbound unit {} to {} percent.",
              debug_string( id ), outbound.percent );
    if( outbound.percent >= 1.0 ) {
      // FIXME: temporary; also, would want to use coroutine ver-
      // sion of this function.
      unit_to_map_square_no_ui( GameState::units(), map_updater,
                                id, Coord{} );
      unit_from_id( id ).clear_orders();
      lg.debug( "unit has arrived in new world." );
      return e_high_seas_result::arrived_in_new_world;
    }
    return e_high_seas_result::still_traveling;
  }
  if_get( info, UnitHarborViewState::inbound, inbound ) {
    inbound.percent += advance;
    inbound.percent = std::clamp( inbound.percent, 0.0, 1.0 );
    lg.debug( "advancing inbound unit {} to {} percent.",
              debug_string( id ), inbound.percent );
    if( inbound.percent >= 1.0 ) {
      GameState::units().change_to_harbor_view(
          id, UnitHarborViewState::in_port{} );
      lg.debug( "unit has arrived in old world." );
      return e_high_seas_result::arrived_in_harbor;
    }
    return e_high_seas_result::still_traveling;
  }
  FATAL( "{} is not on the high seas.", debug_string( id ) );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( create_unit_in_port, UnitId, e_nation nation,
        UnitComposition comp ) {
  auto id = create_unit( GameState::units(), nation,
                         std::move( comp ) );
  GameState::units().change_to_harbor_view(
      id, UnitHarborViewState::in_port{} );
  lg.info( "created a {} in {} port/dock.",
           unit_attr( comp.type() ).name,
           nation_obj( nation ).adjective );
  return id;
}

LUA_FN( unit_sail_to_new_world, void, UnitId id ) {
  unit_sail_to_new_world( id );
}

LUA_FN( advance_unit_on_high_seas, void, UnitId id ) {
  // FIXME
  NonRenderingMapUpdater map_updater( GameState::terrain() );
  advance_unit_on_high_seas( id, map_updater );
}

} // namespace

} // namespace rn
