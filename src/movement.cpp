/****************************************************************
* movement.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Ownership, evolution and movement of units.
*
*****************************************************************/
#include "movement.hpp"

#include "base-util.hpp"
#include "id.hpp"
#include "macros.hpp"
#include "ownership.hpp"
#include "world.hpp"

using namespace std;

namespace rn {

namespace {

} // namespace

// Called at the beginning of each turn; marks all units
// as not yet having moved.
void reset_moves() {
  map_units( []( Unit& unit ) {
    unit.new_turn();
  });
}

// This function will allow the move by default, and so it is the
// burden of the logic in this function to find every possible
// way that the move is *not* allowed and to flag it if that is
// the case.
UnitMoveDesc move_consequences( UnitId id, Coord coords ) {
  Y y = coords.y;
  X x = coords.x;

  auto& unit = unit_from_id( id );
  ASSERT( !unit.moved_this_turn() );

  MovementPoints cost( 1 );

  UnitMoveDesc result{
    coords, false, e_unit_mv_good::map_to_map, cost, UnitId{0}
  };

  if( unit.movement_points() < cost ) {
    result.desc = e_unit_mv_error::insufficient_movement_points;
    return result;
  }

  if( !coords.is_inside( world_rect() ) ) {
    result.desc = e_unit_mv_error::map_edge;
    return result;
  }
  auto& square = square_at( y, x );

  if( unit.desc().boat && square.land ) {
    result.desc = e_unit_mv_error::land_forbidden;
    return result;
  }

  if( !unit.desc().boat && !square.land ) {
    auto const& ships = units_from_coord( y, x );
    if( ships.empty() ) {
      result.desc = e_unit_mv_error::water_forbidden;
      return result;
    }
    // We have at least on ship, so iterate through and find the
    // first one (if any) that the unit can board.
    for( auto ship_id : ships ) {
      auto const& ship_unit = unit_from_id( ship_id );
      ASSERT( ship_unit.desc().boat );
      auto& cargo = ship_unit.cargo();
      if( cargo.fits( id ) ) {
        result.desc = e_unit_mv_good::board_ship;
        result.can_move = true;
        result.target_unit = ship_id;
        return result;
      }
    }
    result.desc = e_unit_mv_error::board_ship_full;
    return result;
  }
  result.desc = e_unit_mv_good::map_to_map;
  result.can_move = true;
  return result;
}

void move_unit_to( UnitId id, Coord target ) {
  auto& unit = unit_from_id( id );
  ASSERT( !unit.moved_this_turn() );

  ASSERT( unit.orders() == e_unit_orders::none );

  UnitMoveDesc move_desc = move_consequences( id, target );
  // Caller should have checked this.
  ASSERT( move_desc.can_move );
  ASSERT( holds_alternative<e_unit_mv_good>( move_desc.desc ) );

  e_unit_mv_good outcome = get<e_unit_mv_good>( move_desc.desc );

  switch( outcome ) {
    case e_unit_mv_good::map_to_map:
      ownership_change_to_map( id, target );
      unit.consume_mv_points( move_desc.movement_cost );
      break;
    case e_unit_mv_good::board_ship:
      ownership_change_to_cargo( move_desc.target_unit, id );
      unit.forfeight_mv_points();
      unit.sentry();
      break;
  }
}

} // namespace rn
