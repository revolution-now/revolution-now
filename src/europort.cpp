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
#include "ownership.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/variant.hpp"

// Range-v3
#include "range/v3/action/sort.hpp"
#include "range/v3/view/filter.hpp"

namespace rn {

namespace {

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

int unit_arrival_id_throw( UnitId id ) {
  ASSIGN_CHECK_OPT( info, unit_euro_port_view_info( id ) );
  GET_CHECK_VARIANT( in_port, info.get(),
                     UnitEuroPortViewState::in_port );
  return in_port.global_arrival_id;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
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

} // namespace rn
