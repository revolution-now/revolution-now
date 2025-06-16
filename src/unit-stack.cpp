/****************************************************************
**unit-stack.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-07.
*
* Description: Handles things related to stacks of units on the
*              same tile.
*
*****************************************************************/
#include "unit-stack.hpp"

// Revolution Now
#include "colony-mgr.hpp"
#include "irand.hpp"

// config
#include "config/natives.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/native-unit.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

int unit_defense_value( SSConst const& ss, GenericUnitId id ) {
  switch( ss.units.unit_kind( id ) ) {
    case e_unit_kind::euro: {
      Unit const& unit = ss.units.euro_unit_for( id );
      return unit.desc().combat;
    }
    case e_unit_kind::native:
      return unit_attr( ss.units.native_unit_for( id ).type )
          .combat;
  }
}

// Sorting order:
//
//   1. decreasing defense value.
//   2. increasing id
//
auto key_func( SSConst const& ss, GenericUnitId id ) {
  return tuple{ unit_defense_value( ss, id ),
                -to_underlying( id ) };
}

// This is the function that contains the logic that determines
// the order in which units are visually stacked when there are
// multiple on a square, and also how a defending unit is chosen
// when attacking a square with multiple units. This is factored
// out into a common location because it needs to be consistent
// across various places in the code base in order to ensure a
// consistent UI/UX.
//
// This is the comparator that should be used for a sorting func-
// tion that puts units in order that they should appear in a
// unit stack. It sorts so that the top unit will be first in the
// resulting sorted list.
bool unit_stack_ordering_cmp( SSConst const& ss,
                              GenericUnitId left,
                              GenericUnitId right ) {
  return key_func( ss, left ) > key_func( ss, right );
}

maybe<UnitId> highest_defense_euro_unit_on_square(
    SSConst const& ss, Coord coord,
    base::function_ref<bool( Unit const& ) const> remove ) {
  unordered_set<GenericUnitId> const& units_at_dst_set =
      ss.units.from_coord( coord );
  vector<UnitId> defenders;
  defenders.reserve( units_at_dst_set.size() );
  for( GenericUnitId const generic_id : units_at_dst_set ) {
    UnitId const unit_id =
        ss.units.check_euro_unit( generic_id );
    Unit const& unit = ss.units.unit_for( unit_id );
    if( remove( unit ) ) continue;
    defenders.push_back(
        ss.units.check_euro_unit( generic_id ) );
  }
  if( defenders.empty() ) return nothing;
  // FIXME: We need to take into account combat modifiers here.
  // The strategy guide claims that the OG does that when se-
  // lecting a defender, which would make sense. Performance wise
  // it should be fine because this function is only called when
  // a battle happens (hopefully); i.e., we don't want to be
  // sorting the units based on modifiers just for rendering unit
  // stacks on the map normally, since that would slow down ren-
  // dering.
  sort_euro_unit_stack( ss, defenders );
  CHECK( !defenders.empty() );
  return defenders[0];
}

maybe<UnitId> highest_defense_euro_unit_on_square(
    SSConst const& ss, Coord coord ) {
  return highest_defense_euro_unit_on_square(
      ss, coord, []( Unit const& ) { return false; } );
}

maybe<NativeUnitId> highest_defense_native_unit_on_square(
    SSConst const& ss, Coord coord ) {
  unordered_set<GenericUnitId> const& braves =
      ss.units.from_coord( coord );
  vector<NativeUnitId> native_unit_ids;
  native_unit_ids.reserve( braves.size() );
  for( GenericUnitId generic_id : braves )
    native_unit_ids.push_back(
        ss.units.check_native_unit( generic_id ) );
  sort_native_unit_stack( ss, native_unit_ids );
  if( native_unit_ids.empty() ) return nothing;
  return native_unit_ids[0];
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void sort_unit_stack( SSConst const& ss,
                      vector<GenericUnitId>& units ) {
  sort( units.begin(), units.end(),
        [&]( GenericUnitId left, GenericUnitId right ) {
          return unit_stack_ordering_cmp( ss, left, right );
        } );
}

void sort_native_unit_stack( SSConst const& ss,
                             vector<NativeUnitId>& units ) {
  sort( units.begin(), units.end(),
        [&]( NativeUnitId left, NativeUnitId right ) {
          return unit_stack_ordering_cmp( ss, left, right );
        } );
}

void sort_euro_unit_stack( SSConst const& ss,
                           vector<UnitId>& units ) {
  sort( units.begin(), units.end(),
        [&]( UnitId left, UnitId right ) {
          return unit_stack_ordering_cmp( ss, left, right );
        } );
}

UnitId select_euro_unit_defender( SSConst const& ss,
                                  Coord tile ) {
  // There should not be a colony on this square; if there is
  // then we're supposed to use the other dedicated function.
  CHECK( !ss.colonies.maybe_from_coord( tile ).has_value() );
  UNWRAP_CHECK( defender_id, highest_defense_euro_unit_on_square(
                                 ss, tile ) );
  return defender_id;
}

NativeUnitId select_native_unit_defender( SSConst const& ss,
                                          Coord tile ) {
  UNWRAP_CHECK(
      defender_id,
      highest_defense_native_unit_on_square( ss, tile ) );
  return defender_id;
}

UnitId select_colony_defender( SSConst const& ss, IRand& rand,
                               Colony const& colony ) {
  // Attacking a colony first attacks all military units at the
  // gate (including scouts), then once those are gone, the next
  // attack will select a random colony worker.
  //
  // NOTE: Non-military units at the gate are never chosen. When
  // the colony is attacked by another european it actually
  // doesn't matter which non-combatant is chosen (whether colony
  // worker or at the gate) because they all have the same combat
  // value (=1), and if the attacker wins they will take the
  // colony without destroying the defending unit (thus it is of
  // no consequence which unit defends). However, it matters for
  // brave attacks. When a brave attacks an undefended colony it
  // destoys the units that it attacks (when it wins and the
  // player has more than one colony) since it can never capture
  // the colony. Thus it matters which non-combatant defends,
  // since it may get destroyed.
  //
  // In the OG the non-combatant defenders are always colony
  // workers as opposed to units at the gate. This is probably
  // good otherwise the player would be able to defend their
  // colony against brave attacks and prevent valuable colony
  // workers from being destroyed by e.g. just putting a bunch of
  // petty criminals in each colony and leaving them at the gate
  // to be sacrified on each brave attack.
  //
  // NOTE: Military units that are in the cargo of ships in the
  // port of the colony being attacked will have been offboarded
  // first in order to help defend.
  auto const ignore_at_gate = [&]( Unit const& unit ) {
    if( !is_military_unit( unit.type() ) ) return true;
    if( unit.desc().ship ) return true;
    return false;
  };
  maybe<UnitId> const at_gate =
      highest_defense_euro_unit_on_square( ss, colony.location,
                                           ignore_at_gate );
  if( at_gate.has_value() ) return *at_gate;
  vector<UnitId> const workers = colony_workers( colony );
  // The save state validator should ensure that there is at
  // least one worker per colony.
  CHECK( !workers.empty() );
  // As discussed above, it is not inconsequential which colony
  // worker we choose (otherwise we'd just choose the first),
  // since said colonist might get destroyed if this is a brave
  // attack. The OG selects a random worker, so we do that.
  return rand.pick_one( workers );
}

} // namespace rn
