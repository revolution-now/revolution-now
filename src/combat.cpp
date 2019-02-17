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
#include "logging.hpp"
#include "ownership.hpp"

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
Opt<CombatAnalysis> combat_impl( UnitId id, Orders orders ) {
  if( !util::holds<orders::direction>( orders ) ) return nullopt;
  auto [direction] = get<orders::direction>( orders );

  auto src_coord = coords_for_unit( id );
  auto dst_coord = src_coord.moved( direction );

  auto& unit = unit_from_id( id );
  CHECK( !unit.moved_this_turn() );

  if( is_unit_onboard( id ) )
    return CombatAnalysis{
        /*id_=*/id,
        /*orders_=*/orders,
        /*units_to_prioritize_=*/{},
        /*attack_src_=*/src_coord,
        /*attack_target_=*/dst_coord,
        /*desc_=*/e_attack_error::attack_from_ship,
        /*target_unit_=*/{},
        /*fight_stats_=*/{}};

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

  auto units_at_dst = units_from_coord( dst_coord );
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
  logger->info( "unit in target square with highest defense: {}",
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
            /*fight_stats_=*/{}};
      case +bh_t::attack:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_good::eu_land_unit,
            /*target_unit_=*/highest_defense_unit,
            /*fight_stats_=*/run_stats()};
      case +bh_t::no_bombard:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_error::ship_attack_land_unit,
            /*target_unit_=*/{},
            /*fight_stats_=*/{}};
      case +bh_t::bombard:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_good::eu_land_unit,
            /*target_unit_=*/highest_defense_unit,
            /*fight_stats_=*/run_stats()};
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
            /*fight_stats_=*/{}};
      case +bh_t::attack:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_good::ship,
            /*target_unit_=*/highest_defense_unit,
            /*fight_stats_=*/run_stats()};
      case +bh_t::no_bombard:;
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_error::land_unit_attack_ship,
            /*target_unit_=*/{},
            /*fight_stats_=*/{}};
      case +bh_t::bombard:;
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_good::ship,
            /*target_unit_=*/highest_defense_unit,
            /*fight_stats_=*/run_stats()};
    }
  }

  // If one of these triggers then that means that:
  //
  //   1) The line should be removed
  //   2) Some logic should be added above to deal
  //      with that particular situation.
  //   3) Don't do #1 without doing #2
  //
  STATIC_ASSERT_NO_BEHAVIOR( land, foreign, colony );
  STATIC_ASSERT_NO_BEHAVIOR( land, foreign, village );

  SHOULD_NOT_BE_HERE;
}

// This is the entry point; calls the implementation then checks
// invariants. Finally computes the combat statistics if the at-
// tack is allowed.
Opt<CombatAnalysis> CombatAnalysis::analyze_( UnitId id,
                                              Orders orders ) {
  auto maybe_res = combat_impl( id, orders );
  if( !maybe_res.has_value() ) return maybe_res;
  auto const& res = *maybe_res;
  // Now check invariants.
  CHECK( res.id == id );
  CHECK( res.attack_src != res.attack_target );
  CHECK( find( res.units_to_prioritize.begin(),
               res.units_to_prioritize.end(),
               id ) == res.units_to_prioritize.end() );
  CHECK( res.attack_src == coords_for_unit( id ) );
  CHECK( res.attack_src.is_adjacent_to( res.attack_target ) );
  CHECK( res.target_unit != id );

  if( res.allowed() ) {
    CHECK( res.target_unit.has_value() );
    CHECK( res.fight_stats.has_value() );
  }

  return maybe_res;
}

bool CombatAnalysis::confirm_explain_() const {
  if( !allowed() ) return false;
  // The above should have checked that the variant holds the
  // e_attack_good type for us.
  // auto& kind = val_or_die<e_attack_good>( desc );

  // switch( kind ) {
  //  case e_attack_good::eu_land_unit:
  //  case e_attack_good::ship:;
  //}
  // SHOULD_NOT_BE_HERE;
  return true;
}

void CombatAnalysis::affect_orders_() const {
  auto& unit = unit_from_id( id );

  CHECK( !unit.moved_this_turn() );
  CHECK( unit.orders() == Unit::e_orders::none );
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
    case e_attack_good::eu_land_unit:
    case e_attack_good::ship:;
  }

  switch( loser.desc().on_death ) {
    case +e_unit_death::destroy: //
      destroy_unit( loser.id() );
      break;
    case +e_unit_death::naval: //
      destroy_unit( loser.id() );
      break;
    case +e_unit_death::capture:
      // Capture only happens to defenders.
      if( loser.id() == defender.id() ) {
        loser.change_nation( winner.nation() );
        move_unit_from_map_to_map(
            loser.id(), coords_for_unit( winner.id() ) );
        if( !loser.moved_this_turn() )
          loser.forfeight_mv_points();
        loser.finish_turn();
        loser.clear_orders();
      }
      break;
    case +e_unit_death::demote:
      CHECK( loser.desc().demoted.has_value() );
      loser.change_type( loser.desc().demoted.value() );
      break;
    case +e_unit_death::maybe_demote:
      // This would be for units that only demote probabilisti-
      // cally.
      NOT_IMPLEMENTED;
      break;
    case +e_unit_death::demote_and_capture:
      CHECK( loser.desc().demoted.has_value() );
      loser.change_type( loser.desc().demoted.value() );
      // Capture only happens to defenders.
      if( loser.id() == defender.id() ) {
        loser.change_nation( winner.nation() );
        move_unit_from_map_to_map(
            loser.id(), coords_for_unit( winner.id() ) );
        if( !loser.moved_this_turn() )
          loser.forfeight_mv_points();
        loser.finish_turn();
        loser.clear_orders();
      }
      break;
  }
}

} // namespace rn
