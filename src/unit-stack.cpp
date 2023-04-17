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
                              GenericUnitId  left,
                              GenericUnitId  right ) {
  return key_func( ss, left ) > key_func( ss, right );
}

maybe<UnitId> highest_defense_euro_unit_on_square(
    SSConst const& ss, Coord coord,
    base::function_ref<bool( UnitId )> remove ) {
  unordered_set<GenericUnitId> const& units_at_dst_set =
      ss.units.from_coord( coord );
  vector<UnitId> defenders;
  defenders.reserve( units_at_dst_set.size() );
  for( GenericUnitId generic_id : units_at_dst_set ) {
    UnitId const unit_id =
        ss.units.check_euro_unit( generic_id );
    if( remove( unit_id ) ) continue;
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
      ss, coord, []( UnitId ) { return false; } );
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
void sort_unit_stack( SSConst const&         ss,
                      vector<GenericUnitId>& units ) {
  sort( units.begin(), units.end(),
        [&]( GenericUnitId left, GenericUnitId right ) {
          return unit_stack_ordering_cmp( ss, left, right );
        } );
}

void sort_native_unit_stack( SSConst const&        ss,
                             vector<NativeUnitId>& units ) {
  sort( units.begin(), units.end(),
        [&]( NativeUnitId left, NativeUnitId right ) {
          return unit_stack_ordering_cmp( ss, left, right );
        } );
}

void sort_euro_unit_stack( SSConst const&  ss,
                           vector<UnitId>& units ) {
  sort( units.begin(), units.end(),
        [&]( UnitId left, UnitId right ) {
          return unit_stack_ordering_cmp( ss, left, right );
        } );
}

UnitId select_euro_unit_defender( SSConst const& ss,
                                  Coord          tile ) {
  // There should not be a colony on this square; if there is
  // then we're supposed to use the other dedicated function.
  CHECK( !ss.colonies.maybe_from_coord( tile ).has_value() );
  UNWRAP_CHECK( defender_id, highest_defense_euro_unit_on_square(
                                 ss, tile ) );
  return defender_id;
}

NativeUnitId select_native_unit_defender( SSConst const& ss,
                                          Coord          tile ) {
  UNWRAP_CHECK(
      defender_id,
      highest_defense_native_unit_on_square( ss, tile ) );
  return defender_id;
}

UnitId select_colony_defender( SSConst const& ss,
                               Colony const&  colony ) {
  // Attacking a colony first attacks all military units, then
  // once those are gone, the next attack will attack either a
  // non-military unit at the gate (if there is one) or a
  // colonist working in the colony otherwise. In either case, if
  // attacking a non-military unit, then the colony will be cap-
  // tured if the attack succeeds.
  maybe<UnitId> const at_gate =
      highest_defense_euro_unit_on_square(
          ss, colony.location, /*remove=*/[&]( UnitId unit_id ) {
            Unit const& unit = ss.units.unit_for( unit_id );
            // TODO: Figure out how to deal with military units
            // that are in the cargo of ships in the port of the
            // colony being attacked. This issue doesn't come up
            // in the original game. Maybe Paul Revere could
            // enable them to fight?
            return unit.desc().ship;
          } );
  if( at_gate.has_value() ) return *at_gate;
  vector<UnitId> const workers = colony_workers( colony );
  CHECK( !workers.empty() );
  return workers[0];
}

} // namespace rn
