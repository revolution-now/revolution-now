/****************************************************************
**movement.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Ownership, evolution and movement of units.
*
*****************************************************************/
#include "movement.hpp"

#include "util.hpp"
#include "id.hpp"
#include "macros.hpp"
#include "ownership.hpp"
#include "world.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

// Called at the beginning of each turn; marks all units
// as not yet having moved.
void reset_moves() {
  map_units( []( Unit& unit ) { unit.new_turn(); } );
}

// This function will allow the move by default, and so it is the
// burden of the logic in this function to find every possible
// way that the move is *not* allowed and to flag it if that is
// the case.
UnitMoveDesc move_consequences( UnitId       id,
                                Coord const& coords ) {
  Y y = coords.y;
  X x = coords.x;

  auto& unit = unit_from_id( id );
  CHECK( !unit.moved_this_turn() );

  MovementPoints cost( 1 );

  UnitMoveDesc result{
      coords, false,     e_unit_mv_good::map_to_map,
      cost,   UnitId{0}, {}};

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
    std::vector<UnitId> to_offload;
    for( auto cargo_id : unit.cargo().items_of_type<UnitId>() ) {
      auto const& cargo_unit = unit_from_id( cargo_id );
      if( !cargo_unit.moved_this_turn() )
        to_offload.push_back( cargo_id );
    }
    if( !to_offload.empty() ) {
      // We have at least one unit in the cargo that is able to
      // make landfall. So we will indicate that the unit is
      // allowed to make this move, but we change the target
      // square to where the unit currently is since it will
      // not physically move.
      result.desc          = e_unit_mv_good::land_fall;
      result.can_move      = true;
      result.coords        = coords_for_unit( unit.id() );
      result.movement_cost = 0;
      result.to_prioritize = to_offload;
      return result;
    }
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
      CHECK( ship_unit.desc().boat );
      auto& cargo = ship_unit.cargo();
      if( cargo.fits( id ) ) {
        result.desc          = e_unit_mv_good::board_ship;
        result.can_move      = true;
        result.target_unit   = ship_id;
        result.to_prioritize = {ship_id};
        return result;
      }
    }
    result.desc = e_unit_mv_error::board_ship_full;
    return result;
  }

  // `holder` will be a valid value if the unit is cargo of an-
  // other unit; the holder's id in that case will be *holder.
  auto holder = is_unit_onboard( unit.id() );
  if( !unit.desc().boat && square.land && holder ) {
    // We have a unit onboard a ship moving onto land.
    result.desc     = e_unit_mv_good::offboard_ship;
    result.can_move = true;
    return result;
  }

  result.desc     = e_unit_mv_good::map_to_map;
  result.can_move = true;
  return result;
}

void move_unit( UnitId id, UnitMoveDesc const& move_desc ) {
  auto& unit = unit_from_id( id );
  CHECK( !unit.moved_this_turn() );

  CHECK( unit.orders() == e_unit_orders::none );

  // Caller should have checked this.
  CHECK( move_desc.can_move );
  CHECK( holds_alternative<e_unit_mv_good>( move_desc.desc ) );

  e_unit_mv_good outcome = get<e_unit_mv_good>( move_desc.desc );

  switch( outcome ) {
    case e_unit_mv_good::map_to_map:
      // If it's a ship then sentry all its units before it
      // moves.
      if( unit.desc().boat ) {
        for( UnitId id : unit.cargo().items_of_type<UnitId>() ) {
          auto& cargo_unit = unit_from_id( id );
          cargo_unit.sentry();
        }
      }
      ownership_change_to_map( id, move_desc.coords );
      unit.consume_mv_points( move_desc.movement_cost );
      break;
    case e_unit_mv_good::board_ship:
      ownership_change_to_cargo( move_desc.target_unit, id );
      unit.forfeight_mv_points();
      unit.sentry();
      break;
    case e_unit_mv_good::offboard_ship:
      ownership_change_to_map( id, move_desc.coords );
      unit.forfeight_mv_points();
      CHECK( unit.orders() == e_unit_orders::none );
      break;
    case e_unit_mv_good::land_fall:
      // Just activate all the units on the ship that have not
      // completed their turns. Note that the ship's movement
      // points are not consumed.
      for( auto cargo_id :
           unit.cargo().items_of_type<UnitId>() ) {
        auto& cargo_unit = unit_from_id( cargo_id );
        if( !cargo_unit.moved_this_turn() ) {
          cargo_unit.clear_orders();
          // In case the unit has already been processed in the
          // turn loop and was passed over due to sentry status
          // onboard (and therefore marked as having finished its
          // turn) while still having movement points left. Since
          // the unit is now being re-prioritized to the begin-
          // ning of the turn loop, we need to mark it's turn as
          // unfinished again this turn so that it will take or-
          // ders otherwise it will just be passed over again
          // this turn.
          cargo_unit.unfinish_turn();
        }
      }
      break;
  }
}

} // namespace rn
