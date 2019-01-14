/****************************************************************
**travel.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Physical movement of units from one square to
*              another.
*
*****************************************************************/
#include "travel.hpp"

// Revolution Now
#include "errors.hpp"
#include "id.hpp"
#include "orders.hpp"
#include "ownership.hpp"
#include "util.hpp"
#include "utype.hpp"
#include "window.hpp"
#include "world.hpp"

// base-util
#include "base-util/variant.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

bool TravelAnalysis::allowed_() const {
  return util::holds<e_unit_travel_good>( desc );
}

void analyze_unload( Unit const&     unit,
                     TravelAnalysis& analysis ) {
  std::vector<UnitId> to_offload;
  for( auto cargo_id : unit.cargo().items_of_type<UnitId>() ) {
    auto const& cargo_unit = unit_from_id( cargo_id );
    if( !cargo_unit.moved_this_turn() )
      to_offload.push_back( cargo_id );
  }
  if( !to_offload.empty() ) {
    // We have at least one unit in the cargo that is able
    // to make landfall. So we will indicate that the unit
    // is al- lowed to make this move.
    analysis.desc                = e_unit_travel_good::land_fall;
    analysis.units_to_prioritize = to_offload;
  } else {
    analysis.desc = e_unit_travel_error::land_forbidden;
  }
}

// This function will allow the move by default, and so it is the
// burden of the logic in this function to find every possible
// way that the move is *not* allowed (among the situations that
// this function is concerned about) and to flag it if that is
// the case.
Opt<TravelAnalysis> analyze_impl( UnitId id, Orders orders ) {
  if( !util::holds<orders::direction>( orders ) ) return nullopt;
  auto [direction] = get<orders::direction>( orders );

  auto src_coord = coords_for_unit( id );
  auto dst_coord = src_coord.moved( direction );

  if( !dst_coord.is_inside( world_rect() ) ) {
    return TravelAnalysis{
        /*id_=*/id,
        /*orders_=*/orders,
        /*units_to_prioritize_=*/{},
        /*unit_would_move_=*/{},
        /*move_src_=*/src_coord,
        /*move_target_=*/dst_coord,
        /*desc_=*/e_unit_travel_error::map_edge,
        /*target_unit=*/{}};
  }
  auto& square = square_at( dst_coord );

  auto& unit = unit_from_id( id );
  CHECK( !unit.moved_this_turn() );

  auto crust = square.crust;

  e_unit_relationship relationship =
      e_unit_relationship::neutral;
  if( auto dst_nation = nation_from_coord( dst_coord );
      dst_nation.has_value() ) {
    if( *dst_nation == unit.nation() )
      relationship = e_unit_relationship::friendly;
    else
      SHOULD_NOT_BE_HERE;
  }

  auto units_at_dst = units_from_coord( dst_coord );

  e_entity_category category = e_entity_category::empty;
  if( !units_at_dst.empty() ) category = e_entity_category::unit;

  // We are entering an empty land square.
  IF_BEHAVIOR( land, neutral, empty ) {
    using bh_t = decltype( bh );
    switch( bh ) {
      case +bh_t::never:
        return TravelAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*unit_would_move_=*/{},
            /*move_src_=*/src_coord,
            /*move_target_=*/dst_coord,
            /*desc_=*/
            e_unit_travel_error::land_forbidden,
            /*target_unit=*/{}};
      case +bh_t::always:
        // `holder` will be a valid value if the unit
        // is cargo of an- other unit; the holder's id
        // in that case will be *holder.
        if( auto holder = is_unit_onboard( unit.id() );
            holder ) {
          // We have a unit onboard a ship moving onto
          // land.
          return TravelAnalysis{
              /*id_=*/id,
              /*orders_=*/orders,
              /*units_to_prioritize_=*/{},
              /*unit_would_move_=*/true,
              /*move_src_=*/src_coord,
              /*move_target_=*/dst_coord,
              /*desc_=*/
              e_unit_travel_good::offboard_ship,
              /*target_unit=*/{}};
        } else {
          return TravelAnalysis{
              /*id_=*/id,
              /*orders_=*/orders,
              /*units_to_prioritize_=*/{},
              /*unit_would_move_=*/true,
              /*move_src_=*/src_coord,
              /*move_target_=*/dst_coord,
              /*desc_=*/e_unit_travel_good::map_to_map,
              /*target_unit=*/{}};
        }
      case +bh_t::unload: {
        auto res = TravelAnalysis{/*id_=*/id,
                                  /*orders_=*/orders,
                                  /*units_to_prioritize_=*/{},
                                  /*unit_would_move_=*/false,
                                  /*move_src_=*/src_coord,
                                  /*move_target_=*/dst_coord,
                                  /*desc_=*/{},
                                  /*target_unit=*/{}};
        analyze_unload( unit, res );
        return res;
      }
    }
  }
  // We are entering a land square containing a friendly unit.
  IF_BEHAVIOR( land, friendly, unit ) {
    using bh_t = decltype( bh );
    switch( bh ) {
      case +bh_t::never:
        return TravelAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*unit_would_move_=*/{},
            /*move_src_=*/src_coord,
            /*move_target_=*/dst_coord,
            /*desc_=*/
            e_unit_travel_error::land_forbidden,
            /*target_unit=*/{}};
      case +bh_t::always:
        // `holder` will be a valid value if the unit
        // is cargo of an- other unit; the holder's id
        // in that case will be *holder.
        if( auto holder = is_unit_onboard( unit.id() );
            holder ) {
          // We have a unit onboard a ship moving onto
          // land.
          return TravelAnalysis{
              /*id_=*/id,
              /*orders_=*/orders,
              /*units_to_prioritize_=*/{},
              /*unit_would_move_=*/true,
              /*move_src_=*/src_coord,
              /*move_target_=*/dst_coord,
              /*desc_=*/
              e_unit_travel_good::offboard_ship,
              /*target_unit=*/{}};
        } else {
          return TravelAnalysis{
              /*id_=*/id,
              /*orders_=*/orders,
              /*units_to_prioritize_=*/{},
              /*unit_would_move_=*/true,
              /*move_src_=*/src_coord,
              /*move_target_=*/dst_coord,
              /*desc_=*/e_unit_travel_good::map_to_map,
              /*target_unit=*/{}};
        }
      case +bh_t::unload: {
        auto res = TravelAnalysis{/*id_=*/id,
                                  /*orders_=*/orders,
                                  /*units_to_prioritize_=*/{},
                                  /*unit_would_move_=*/false,
                                  /*move_src_=*/src_coord,
                                  /*move_target_=*/dst_coord,
                                  /*desc_=*/{},
                                  /*target_unit=*/{}};
        analyze_unload( unit, res );
        return res;
      }
    }
  }
  // We are entering an empty water square.
  IF_BEHAVIOR( water, neutral, empty ) {
    using bh_t = decltype( bh );
    switch( bh ) {
      case +bh_t::never:
        return TravelAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*unit_would_move_=*/{},
            /*move_src_=*/src_coord,
            /*move_target_=*/dst_coord,
            /*desc_=*/
            e_unit_travel_error::water_forbidden,
            /*target_unit=*/{}};
      case +bh_t::always:
        return TravelAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*unit_would_move_=*/true,
            /*move_src_=*/src_coord,
            /*move_target_=*/dst_coord,
            /*desc_=*/e_unit_travel_good::map_to_map,
            /*target_unit=*/{}};
    }
  }
  // We are entering a water square containing a friendly unit.
  IF_BEHAVIOR( water, friendly, unit ) {
    using bh_t = decltype( bh );
    switch( bh ) {
      case +bh_t::never:
        return TravelAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*unit_would_move_=*/{},
            /*move_src_=*/src_coord,
            /*move_target_=*/dst_coord,
            /*desc_=*/
            e_unit_travel_error::water_forbidden,
            /*target_unit=*/{}};
      case +bh_t::always:
        return TravelAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*unit_would_move_=*/true,
            /*move_src_=*/src_coord,
            /*move_target_=*/dst_coord,
            /*desc_=*/e_unit_travel_good::map_to_map,
            /*target_unit=*/{}};
      case +bh_t::move_onto_ship: {
        auto const& ships = units_at_dst;
        if( ships.empty() ) {
          return TravelAnalysis{
              /*id_=*/id,
              /*orders_=*/orders,
              /*units_to_prioritize_=*/{},
              /*unit_would_move_=*/{},
              /*move_src_=*/src_coord,
              /*move_target_=*/dst_coord,
              /*desc_=*/
              e_unit_travel_error::water_forbidden,
              /*target_unit=*/{}};
        }
        // We have at least one ship, so iterate
        // through and find the first one (if any) that
        // the unit can board.
        for( auto ship_id : ships ) {
          auto const& ship_unit = unit_from_id( ship_id );
          CHECK( ship_unit.desc().boat );
          if( auto const& cargo = ship_unit.cargo();
              cargo.fits( id ) ) {
            return TravelAnalysis{
                /*id_=*/id,
                /*orders_=*/orders,
                /*units_to_prioritize_=*/{ship_id},
                /*unit_would_move_=*/true,
                /*move_src_=*/src_coord,
                /*move_target_=*/dst_coord,
                /*desc_=*/
                e_unit_travel_good::board_ship,
                /*target_unit=*/ship_id};
          }
        }
        return TravelAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*unit_would_move_=*/{},
            /*move_src_=*/src_coord,
            /*move_target_=*/dst_coord,
            /*desc_=*/
            e_unit_travel_error::board_ship_full,
            /*target_unit=*/{}};
      }
    }
  }

  // If one of these triggers then that means that:
  //
  //   1) The line should be removed
  //   2) Some logic should be added above to deal
  //      with that particular situation.
  //   3) Don't do #1 without doing #2
  //
  STATIC_ASSERT_NO_BEHAVIOR( land, friendly, colony );
  STATIC_ASSERT_NO_BEHAVIOR( land, friendly, empty );
  STATIC_ASSERT_NO_BEHAVIOR( land, friendly, village );
  STATIC_ASSERT_NO_BEHAVIOR( land, neutral, colony );
  STATIC_ASSERT_NO_BEHAVIOR( land, neutral, unit );
  STATIC_ASSERT_NO_BEHAVIOR( land, neutral, village );
  STATIC_ASSERT_NO_BEHAVIOR( water, friendly, colony );
  STATIC_ASSERT_NO_BEHAVIOR( water, friendly, empty );
  STATIC_ASSERT_NO_BEHAVIOR( water, friendly, village );
  STATIC_ASSERT_NO_BEHAVIOR( water, neutral, colony );
  STATIC_ASSERT_NO_BEHAVIOR( water, neutral, unit );
  STATIC_ASSERT_NO_BEHAVIOR( water, neutral, village );

  SHOULD_NOT_BE_HERE;
}

// This is the entry point; calls the implementation then checks
// invariants.
Opt<TravelAnalysis> TravelAnalysis::analyze_( UnitId id,
                                              Orders orders ) {
  auto maybe_res = analyze_impl( id, orders );
  if( !maybe_res.has_value() ) return maybe_res;
  auto const& res = *maybe_res;
  // Now check invariants.
  CHECK( res.id == id );
  CHECK( res.move_src != res.move_target );
  CHECK( find( res.units_to_prioritize.begin(),
               res.units_to_prioritize.end(),
               id ) == res.units_to_prioritize.end() );
  CHECK( res.move_src == coords_for_unit( id ) );
  CHECK( res.move_src.is_adjacent_to( res.move_target ) );
  CHECK( res.target_unit != id );
  return maybe_res;
}

void TravelAnalysis::affect_orders_() const {
  auto& unit = unit_from_id( id );

  CHECK( !unit.moved_this_turn() );
  CHECK( unit.orders() == Unit::e_orders::none );
  CHECK( allowed() );

  e_unit_travel_good outcome = get<e_unit_travel_good>( desc );

  // This will throw if the unit has no coords, but I think it
  // should always at this point if we're moving it.
  auto old_coord = coords_for_unit( id );

  switch( outcome ) {
    case e_unit_travel_good::map_to_map:
      // If it's a ship then sentry all its units before it
      // moves.
      if( unit.desc().boat ) {
        for( UnitId id : unit.cargo().items_of_type<UnitId>() ) {
          auto& cargo_unit = unit_from_id( id );
          cargo_unit.sentry();
        }
      }
      ownership_change_to_map( id, move_target );
      unit.consume_mv_points( MvPoints( 1 ) );
      break;
    case e_unit_travel_good::board_ship: {
      CHECK( target_unit.has_value() );
      ownership_change_to_cargo( *target_unit, id );
      unit.forfeight_mv_points();
      unit.sentry();
      // If the ship is sentried then clear it's orders because
      // the player will likely want to start moving it now that
      // a unit has boarded.
      auto& ship_unit = unit_from_id( *target_unit );
      ship_unit.clear_orders();
      // The ship may have been marked as having finished its
      // turn while still having movement points left if e.g. it
      // used its turn to sentry while still having points left.
      // In that case let's unfinish its turn so that it will be
      // asked for orders again. There should be no harm in
      // calling this even if the ship has in fact already used
      // its movement points this turn.
      ship_unit.unfinish_turn();
      break;
    }
    case e_unit_travel_good::offboard_ship:
      ownership_change_to_map( id, move_target );
      unit.forfeight_mv_points();
      CHECK( unit.orders() == Unit::e_orders::none );
      break;
    case e_unit_travel_good::land_fall:
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
          auto direction = old_coord.direction_to( move_target );
          CHECK( direction.has_value() );
          Orders orders = orders::direction{*direction};
          push_unit_orders( cargo_id, orders );
        }
      }
      break;
  }

  // Now do a sanity check.
  auto new_coord = coords_for_unit( id );
  CHECK( unit_would_move == ( new_coord == move_target ) );
}

bool TravelAnalysis::confirm_explain_() const {
  if( !allowed() ) return false;
  // The above should have checked that the variant holds the
  // e_unit_travel_good type for us.
  auto& kind = val_or_die<e_unit_travel_good>( desc );

  switch( kind ) {
    case e_unit_travel_good::land_fall: {
      auto answer =
          ui::yes_no( "Would you like to make landfall?" );
      return ( answer == ui::e_confirm::yes );
    }
    case e_unit_travel_good::map_to_map:
    case e_unit_travel_good::board_ship:
    case e_unit_travel_good::offboard_ship:
      // Nothing to ask here, just allow the move.
      return true;
  }
  SHOULD_NOT_BE_HERE;
}

} // namespace rn
