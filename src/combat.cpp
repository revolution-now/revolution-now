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
#include "ownership.hpp"

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

  // Make sure there is a foreign entity in the square otherwise
  // there can be no combat.
  auto dst_nation = nation_from_coord( dst_coord );
  if( !dst_nation.has_value() || *dst_nation == unit.nation() )
    return nullopt;

  if( !dst_coord.is_inside( world_rect() ) ) return nullopt;

  auto& square = square_at( dst_coord );

  auto crust        = square.crust;
  auto relationship = e_unit_relationship::foreign;
  auto category     = e_entity_category::unit;

  auto units_at_dst = units_from_coord( dst_coord );
  CHECK( !units_at_dst.empty() );

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
            /*target_unit=*/{}};
      case +bh_t::attack:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_good::eu_land_unit,
            /*target_unit=*/{}};
      case +bh_t::no_bombard:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_error::ship_attack_land_unit,
            /*target_unit=*/{}};
      case +bh_t::bombard:
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_good::eu_land_unit,
            /*target_unit=*/{}};
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
            /*target_unit=*/{}};
      case +bh_t::attack:
        return CombatAnalysis{/*id_=*/id,
                              /*orders_=*/orders,
                              /*units_to_prioritize_=*/{},
                              /*attack_src_=*/src_coord,
                              /*attack_target_=*/dst_coord,
                              /*desc_=*/e_attack_good::ship,
                              /*target_unit=*/{}};
      case +bh_t::no_bombard:;
        return CombatAnalysis{
            /*id_=*/id,
            /*orders_=*/orders,
            /*units_to_prioritize_=*/{},
            /*attack_src_=*/src_coord,
            /*attack_target_=*/dst_coord,
            /*desc_=*/e_attack_error::land_unit_attack_ship,
            /*target_unit=*/{}};
      case +bh_t::bombard:;
        return CombatAnalysis{/*id_=*/id,
                              /*orders_=*/orders,
                              /*units_to_prioritize_=*/{},
                              /*attack_src_=*/src_coord,
                              /*attack_target_=*/dst_coord,
                              /*desc_=*/e_attack_good::ship,
                              /*target_unit=*/{}};
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
// invariants.
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
  return maybe_res;
}

void CombatAnalysis::affect_orders_() const {
  auto& unit = unit_from_id( id );

  CHECK( !unit.moved_this_turn() );
  CHECK( unit.orders() == Unit::e_orders::none );
  CHECK( allowed() );

  auto outcome = get<e_attack_good>( desc );

  // This will throw if the unit has no coords, but I think it
  // should always at this point if we're attacking with it.
  // auto old_coord = coords_for_unit( id );

  switch( outcome ) {
    case e_attack_good::eu_land_unit:
    case e_attack_good::ship:;
  }
}

bool CombatAnalysis::confirm_explain_() const {
  if( !allowed() ) return false;
  // The above should have checked that the variant holds the
  // e_attack_good type for us.
  auto& kind = val_or_die<e_attack_good>( desc );

  switch( kind ) {
    case e_attack_good::eu_land_unit:
    case e_attack_good::ship:;
  }
  SHOULD_NOT_BE_HERE;
}

} // namespace rn
