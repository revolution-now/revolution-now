/****************************************************************
**combat.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-06.
*
* Description: Handles combat statistics.
*
*****************************************************************/
#include "combat.hpp"

// Revolution Now
#include "cstate.hpp"
#include "logging.hpp"
#include "terrain.hpp"
#include "ustate.hpp"
#include "variant.hpp"
#include "window.hpp"

// base-util
#include "base-util/algo.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

bool CombatAnalysis::allowed_() const {
  return util::holds<e_attack_good>( desc );
}

// This function will allow the move by default, and so it is the
// burden of the logic in this function to find every possible
// way that the move is *not* allowed (among the situations that
// this function is concerned about) and to flag it if that is
// the case.
Opt<CombatAnalysis> combat_impl( UnitId id, orders_t orders ) {
  if( !util::holds<orders::direction>( orders ) ) return nullopt;
  auto [direction] = get<orders::direction>( orders );

  auto src_coord = coord_for_unit_indirect( id );
  auto dst_coord = src_coord.moved( direction );

  auto& unit = unit_from_id( id );
  CHECK( !unit.mv_pts_exhausted() );

  if( is_unit_onboard( id ) )
    return CombatAnalysis{
        /*id_=*/id,
        /*orders_=*/orders,
        /*units_to_prioritize_=*/{},
        /*attack_src_=*/src_coord,
        /*attack_target_=*/dst_coord,
        /*desc_=*/e_attack_error::attack_from_ship,
        /*target_unit_=*/{},
        /*fight_stats_=*/{} };

  // Make sure there is a foreign entity in the square otherwise
  // there can be no combat.
  auto dst_nation = nation_from_coord( dst_coord );
  if( !dst_nation.has_value() || *dst_nation == unit.nation() )
    return nullopt;

  if( !dst_coord.is_inside( world_rect_tiles() ) )
    return nullopt;

  auto& square = square_at( dst_coord );

  auto crust        = square.crust;
  auto relationship = e_unit_relationship::foreign;
  auto category     = e_entity_category::unit;
  if( colony_from_coord( dst_coord ).has_value() )
    category = e_entity_category::colony;

  auto const& units_at_dst_set = units_from_coord( dst_coord );
  Vec<UnitId> units_at_dst( units_at_dst_set.begin(),
                            units_at_dst_set.end() );
  auto        colony_at_dst = colony_from_coord( dst_coord );

  // If we have a colony then we only want to get units that are
  // military units (and not ships), since we want the following
  // behavior: attacking a colony first attacks all military
  // units, then once those are gone, the next attack will attack
  // a colonist working in the colony (and if the attack suc-
  // ceeds, the colony is taken) even if there are free colonists
  // on the colony map square.
  if( colony_at_dst ) {
    util::remove_if( units_at_dst,
                     L( unit_from_id( _ ).desc().ship ) );
    util::remove_if(
        units_at_dst,
        L( !unit_from_id( _ ).desc().is_military_unit() ) );
  }

  // If military units are exhausted then attack the colony.
  if( colony_at_dst && units_at_dst.empty() ) {
    auto const& colony = colony_from_id( *colony_at_dst );
    Vec<UnitId> units_working_in_colony = colony.units();
    CHECK( units_working_in_colony.size() > 0 );
    // Sort since order is otherwise unspecified.
    sort( units_working_in_colony.begin(),
          units_working_in_colony.end() );
    units_at_dst.push_back( units_working_in_colony[0] );
  }
  CHECK( !units_at_dst.empty() );
  // Now let's find the unit with the highest defense points
  // among the units in the target square.
  vector<UnitId> sorted_by_defense( units_at_dst.begin(),
                                    units_at_dst.end() );
  util::sort_by_key(
      sorted_by_defense,
      L( unit_from_id( _ ).desc().defense_points ) );
  CHECK( !sorted_by_defense.empty() );
  UnitId highest_defense_unit = sorted_by_defense.back();
  lg.info( "unit in target square with highest defense: {}",
           debug_string( highest_defense_unit ) );

  // Deferred evaluation until we know that the attack makes
  // sense.
  auto run_stats = [id, highest_defense_unit] {
    return fight_statistics( id, highest_defense_unit );
  };

  // We are entering a land square containing a foreign unit.
  IF_BEHAVIOR( land, foreign, unit ) {
    using bh_t = decltype( bh );
    switch( bh ) {
      case +bh_t::no_attack:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_error::unit_cannot_attack,
            /*target_unit_=*/{},
            /*fight_stats_=*/{} };
      case +bh_t::attack:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_good::eu_land_unit,
            /*target_unit_=*/highest_defense_unit,
            /*fight_stats_=*/run_stats() };
      case +bh_t::no_bombard:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_error::ship_attack_land_unit,
            /*target_unit_=*/{},
            /*fight_stats_=*/{} };
      case +bh_t::bombard:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_good::eu_land_unit,
            /*target_unit_=*/highest_defense_unit,
            /*fight_stats_=*/run_stats() };
    }
  }
  // We are entering a land square containing a foreign unit.
  IF_BEHAVIOR( land, foreign, colony ) {
    using bh_t = decltype( bh );
    switch( bh ) {
      case +bh_t::never:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_error::unit_cannot_attack,
            /*target_unit_=*/{},
            /*fight_stats_=*/{} };
      case +bh_t::attack: {
        e_attack_good which =
            unit_from_id( highest_defense_unit )
                    .desc()
                    .is_military_unit()
                ? e_attack_good::colony_defended
                : e_attack_good::colony_undefended;
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/which,
            /*target_unit_=*/highest_defense_unit,
            /*fight_stats_=*/run_stats() };
      }
      case +bh_t::trade:
        // FIXME: implement trade.
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_error::unit_cannot_attack,
            /*target_unit_=*/{},
            /*fight_stats_=*/{} };
    }
  }
  // We are entering a water square containing a foreign unit.
  IF_BEHAVIOR( water, foreign, unit ) {
    using bh_t = decltype( bh );
    switch( bh ) {
      case +bh_t::no_attack:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_error::unit_cannot_attack,
            /*target_unit_=*/{},
            /*fight_stats_=*/{} };
      case +bh_t::attack:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_good::ship,
            /*target_unit_=*/highest_defense_unit,
            /*fight_stats_=*/run_stats() };
      case +bh_t::no_bombard:;
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_error::land_unit_attack_ship,
            /*target_unit_=*/{},
            /*fight_stats_=*/{} };
      case +bh_t::bombard:;
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_good::ship,
            /*target_unit_=*/highest_defense_unit,
            /*fight_stats_=*/run_stats() };
    }
  }

  // If one of these triggers then that means that:
  //
  //   1) The line should be removed
  //   2) Some logic should be added above to deal
  //      with that particular situation.
  //   3) Don't do #1 without doing #2
  //
  STATIC_ASSERT_NO_BEHAVIOR( land, foreign, village );

  SHOULD_NOT_BE_HERE;
}

// This is the entry point; calls the implementation then checks
// invariants. Finally computes the combat statistics if the at-
// tack is allowed.
Opt<CombatAnalysis> CombatAnalysis::analyze_( UnitId   id,
                                              orders_t orders ) {
  auto maybe_res = combat_impl( id, orders );
  if( !maybe_res.has_value() ) return maybe_res;
  auto const& res = *maybe_res;
  // Now check invariants.
  CHECK( res.id == id );
  CHECK( res.attack_src != res.attack_target );
  CHECK( find( res.units_to_prioritize.begin(),
               res.units_to_prioritize.end(),
               id ) == res.units_to_prioritize.end() );
  CHECK( res.attack_src == coord_for_unit_indirect( id ) );
  CHECK( res.attack_src.is_adjacent_to( res.attack_target ) );
  CHECK( res.target_unit != id );

  if( res.allowed() ) {
    CHECK( res.target_unit.has_value() );
    CHECK( res.fight_stats.has_value() );
  }

  return maybe_res;
}

sync_future<bool> confirm_explain_attack_good(
    e_attack_good val ) {
  switch( val ) {
    case e_attack_good::eu_land_unit:
    case e_attack_good::ship:
    case e_attack_good::colony_defended:
      return make_sync_future<bool>( true );
    case e_attack_good::colony_undefended: {
      auto q = fmt::format(
          "This action may result in the capture of a colony, "
          "which is not yet supported.  Therefore, this move is "
          "cancelled." );
      return ui::message_box( q ).fmap(
          []( auto ) { return false; } );
    }
  }
  UNREACHABLE_LOCATION;
  return make_sync_future<bool>( true );
}

sync_future<bool> confirm_explain_attack_error(
    e_attack_error val ) {
  auto return_false = []( auto ) { return false; };
  switch( val ) {
    case e_attack_error::unit_cannot_attack:
      return ui::message_box( "This unit cannot attack." )
          .fmap( return_false );
    case e_attack_error::land_unit_attack_ship:
      return ui::message_box( "Land units cannot attack ships." )
          .fmap( return_false );
    case e_attack_error::ship_attack_land_unit:
      return ui::message_box( "Ships cannot attack land units." )
          .fmap( return_false );
    case e_attack_error::attack_from_ship:
      return ui::message_box( "Cannot attack from a ship." )
          .fmap( return_false );
  }
  UNREACHABLE_LOCATION;
}

sync_future<bool> CombatAnalysis::confirm_explain_() const {
  return overload_visit<sync_future<bool>>(
      desc,
      []( e_attack_good val ) {
        return confirm_explain_attack_good( val );
      },
      []( e_attack_error val ) {
        return confirm_explain_attack_error( val );
      } );
}

void CombatAnalysis::affect_orders_() const {
  auto& unit = unit_from_id( id );

  CHECK( !unit.mv_pts_exhausted() );
  CHECK( unit.orders() == e_unit_orders::none );
  CHECK( allowed() );
  CHECK( target_unit.has_value() );
  CHECK( fight_stats.has_value() );

  auto allowed_reason = get<e_attack_good>( desc );

  auto& attacker = unit;
  auto& defender = unit_from_id( *target_unit );
  auto& winner =
      fight_stats->attacker_wins ? attacker : defender;
  auto& loser = fight_stats->attacker_wins ? defender : attacker;

  attacker.consume_mv_points( MvPoints( 1 ) );

  switch( allowed_reason ) {
    case e_attack_good::colony_undefended: {
      if( winner.id() == attacker.id() ) {
        TODO( "Capture the colony." );
        return;
      }
      // !! Fallthrough.
    }
    case e_attack_good::colony_defended:
    case e_attack_good::eu_land_unit:
    case e_attack_good::ship:;
  }

  switch( loser.desc().on_death ) {
    case e_unit_death::destroy: //
      destroy_unit( loser.id() );
      break;
    case e_unit_death::naval: //
      destroy_unit( loser.id() );
      break;
    case e_unit_death::capture:
      // Capture only happens to defenders.
      if( loser.id() == defender.id() ) {
        loser.change_nation( winner.nation() );
        move_unit_from_map_to_map(
            loser.id(), coord_for_unit_indirect( winner.id() ) );
        if( !loser.mv_pts_exhausted() )
          loser.forfeight_mv_points();
        loser.finish_turn();
        loser.clear_orders();
      }
      break;
    case e_unit_death::demote:
      CHECK( loser.desc().demoted.has_value() );
      loser.change_type( loser.desc().demoted.value() );
      break;
    case e_unit_death::maybe_demote:
      // This would be for units that only demote probabilisti-
      // cally.
      NOT_IMPLEMENTED;
      break;
    case e_unit_death::demote_and_capture:
      CHECK( loser.desc().demoted.has_value() );
      loser.change_type( loser.desc().demoted.value() );
      // Capture only happens to defenders.
      if( loser.id() == defender.id() ) {
        loser.change_nation( winner.nation() );
        move_unit_from_map_to_map(
            loser.id(), coord_for_unit_indirect( winner.id() ) );
        if( !loser.mv_pts_exhausted() )
          loser.forfeight_mv_points();
        loser.finish_turn();
        loser.clear_orders();
      }
      break;
  }
}

} // namespace rn
