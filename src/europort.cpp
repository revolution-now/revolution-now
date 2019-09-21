/****************************************************************
**europort.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-07-08.
*
* Description:
*
*****************************************************************/
#include "europort.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"
#include "logging.hpp"
#include "lua-ext.hpp"
#include "lua.hpp"
#include "ownership.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/variant.hpp"

// Range-v3
#include "range/v3/action/sort.hpp"
#include "range/v3/view/filter.hpp"

using namespace std;

namespace rn {

namespace {

int unit_arrival_id_throw( UnitId id ) {
  // Until a better method is found to organize units in port.
  return id._;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
bool is_unit_on_dock( UnitId id ) {
  auto europort_status = unit_euro_port_view_info( id );
  return europort_status.has_value() &&
         !unit_from_id( id ).desc().boat &&
         util::holds<UnitEuroPortViewState::in_port>(
             europort_status->get() );
}

bool is_unit_inbound( UnitId id ) {
  auto europort_status = unit_euro_port_view_info( id );
  auto is_inbound      = europort_status.has_value() &&
                    util::holds<UnitEuroPortViewState::inbound>(
                        europort_status->get() );
  if( is_inbound ) { CHECK( unit_from_id( id ).desc().boat ); }
  return is_inbound;
}

bool is_unit_outbound( UnitId id ) {
  auto europort_status = unit_euro_port_view_info( id );
  auto is_outbound =
      europort_status.has_value() &&
      util::holds<UnitEuroPortViewState::outbound>(
          europort_status->get() );
  if( is_outbound ) { CHECK( unit_from_id( id ).desc().boat ); }
  return is_outbound;
}

bool is_unit_in_port( UnitId id ) {
  auto europort_status = unit_euro_port_view_info( id );
  return europort_status.has_value() &&
         unit_from_id( id ).desc().boat &&
         util::holds<UnitEuroPortViewState::in_port>(
             europort_status->get() );
}

Vec<UnitId> europort_units_on_dock() {
  auto        in_euroview = units_in_euro_port_view();
  Vec<UnitId> res = in_euroview | rv::filter( is_unit_on_dock );
  // Now we must order the units by their arrival time in port
  // (or on dock).
  res |= rg::action::sort( std::less{}, unit_arrival_id_throw );
  return res;
}

Vec<UnitId> europort_units_in_port() {
  auto        in_euroview = units_in_euro_port_view();
  Vec<UnitId> res = in_euroview | rv::filter( is_unit_in_port );
  // Now we must order the units by their arrival time in port
  // (or on dock).
  res |= rg::action::sort( std::less{}, unit_arrival_id_throw );
  return res;
}

// To old world.
Vec<UnitId> europort_units_inbound() {
  auto in_euroview = units_in_euro_port_view();
  return in_euroview | rv::filter( is_unit_inbound );
}

// To new world.
Vec<UnitId> europort_units_outbound() {
  auto in_euroview = units_in_euro_port_view();
  return in_euroview | rv::filter( is_unit_outbound );
}

void unit_sail_to_old_world( UnitId id ) {
  // FIXME: do other checks here, e.g., make sure that the
  //        ship is not damaged.
  CHECK( unit_from_id( id ).desc().boat );
  // This is the state to which we will set the unit, at least by
  // default (though it might get modified below based on the
  // current state of the unit).
  UnitEuroPortViewState_t target_state =
      UnitEuroPortViewState::inbound{/*progress=*/0.0};
  auto maybe_state = unit_euro_port_view_info( id );
  if( maybe_state ) {
    switch_( maybe_state->get() ) {
      case_( UnitEuroPortViewState::inbound ) {
        // no-op, i.e., keep state the same.
        target_state = val;
      }
      case_( UnitEuroPortViewState::outbound, percent ) {
        if( percent > 0.0 ) {
          // Unit must "turn around" and go the other way.
          target_state = UnitEuroPortViewState::inbound{
              /*progress=*/( 1.0 - percent )};
        } else {
          // Unit has not yet made any progress, so we can imme-
          // diately move it to in_port.
          target_state = UnitEuroPortViewState::in_port{};
        }
      }
      case_( UnitEuroPortViewState::in_port ) {
        // no-op, unit is already in port.
        target_state = val;
      }
      switch_exhaustive;
    }
  }
  if( maybe_state && target_state == maybe_state->get() ) //
    return;
  lg.info( "setting unit {} to state {}", debug_string( id ),
           target_state );
  // Note: unit may already be in a europort state here.
  ownership_change_to_euro_port_view( id, target_state );
}

void unit_sail_to_new_world( UnitId id ) {
  // FIXME: do other checks here, e.g., make sure that the
  //        ship is not damaged.
  CHECK( unit_from_id( id ).desc().boat );
  // This is the state to which we will set the unit, at least by
  // default (though it might get modified below based on the
  // current state of the unit).
  UnitEuroPortViewState_t target_state =
      UnitEuroPortViewState::outbound{/*progress=*/0.0};
  auto maybe_state = unit_euro_port_view_info( id );
  switch_( maybe_state->get() ) {
    case_( UnitEuroPortViewState::outbound ) {
      // no-op, i.e., keep state the same.
      target_state = val;
    }
    case_( UnitEuroPortViewState::inbound, percent ) {
      if( percent > 0.0 ) {
        // Unit must "turn around" and go the other way.
        target_state = UnitEuroPortViewState::outbound{
            /*progress=*/( 1.0 - percent )};
      } else {
        NOT_IMPLEMENTED; // find a place on the map to move to.
      }
    }
    case_( UnitEuroPortViewState::in_port ) {
      // keep default target.
    }
    switch_exhaustive;
  }
  if( maybe_state && target_state == maybe_state->get() ) //
    return;
  lg.info( "setting unit {} to state {}", debug_string( id ),
           target_state );
  // Note: unit may already be in a europort state here.
  ownership_change_to_euro_port_view( id, target_state );
}

void unit_move_to_europort_dock( UnitId id ) {
  if( is_unit_on_dock( id ) ) return;
  auto holder = is_unit_onboard( id );
  CHECK( holder && is_unit_in_port( *holder ),
         "cannot move unit to dock unless it is in the cargo of "
         "a ship that is in port." );
  ownership_change_to_euro_port_view(
      id, UnitEuroPortViewState::in_port{} );
  DCHECK( is_unit_on_dock( id ) );
  DCHECK( !is_unit_onboard( id ) );
}

void advance_unit_on_high_seas( UnitId id ) {
  ASSIGN_CHECK_OPT( info, unit_euro_port_view_info( id ) );
  constexpr double const advance = 0.2;
  if_v( info.get(), UnitEuroPortViewState::outbound, outbound ) {
    outbound->percent += advance;
    if( outbound->percent >= 1.0 ) {
      NOT_IMPLEMENTED; // find a place on the map to move to.
    }
    return;
  }
  if_v( info.get(), UnitEuroPortViewState::inbound, inbound ) {
    inbound->percent += advance;
    if( inbound->percent >= 1.0 )
      ownership_change_to_euro_port_view(
          id, UnitEuroPortViewState::in_port{} );
    return;
  }
  FATAL( "{} is not on the high seas.", debug_string( id ) );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( create_unit_in_port, UnitId, e_nation nation,
        e_unit_type type ) {
  auto id = create_unit_in_euroview_port( nation, type );
  lg.info( "created a {} on {} dock.", unit_desc( type ).name,
           nation );
  return id;
}

} // namespace

} // namespace rn
