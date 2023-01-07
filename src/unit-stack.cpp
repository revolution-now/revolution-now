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

// config
#include "config/natives.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/native-unit.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

// FIXME: get rid of separate attack/defense points for euro
// units.
int unit_defense_value( SSConst const& ss, GenericUnitId id ) {
  switch( ss.units.unit_kind( id ) ) {
    case e_unit_kind::euro:
      return ss.units.euro_unit_for( id ).desc().defense_points;
    case e_unit_kind::native:
      return unit_attr( ss.units.native_unit_for( id ).type )
          .combat;
  }
}

int is_ship( SSConst const& ss, GenericUnitId id ) {
  switch( ss.units.unit_kind( id ) ) {
    case e_unit_kind::euro:
      return ss.units.euro_unit_for( id ).desc().ship;
    case e_unit_kind::native: return false;
  }
}

// Sorting order:
//
//   1. force_top if specified.
//   2. ships.
//   3. defense value.
//   4. id
//
auto key_func( SSConst const& ss, GenericUnitId id,
               maybe<GenericUnitId> force_top ) {
  return tuple{ force_top == id, is_ship( ss, id ),
                unit_defense_value( ss, id ), id };
}

// This is the function that contains the logic that determines
// the order in which units are visually stacked when there are
// multiple on a square, and also how a defending unit is chosen
// when attacking a square with multiple units. This is factored
// out into a common location because it needs to be consistent
// across various places in the code base in order to ensure a
// consistent UI/UX.
//
// `force_top` is if there is one unit that we know that we want
// to force on the top
//
// This is the comparator that should be used for a sorting func-
// tion that puts units in order that they should appear in a
// unit stack. It sorts so that the top unit will be first in the
// resulting sorted list.
bool unit_stack_ordering_cmp( SSConst const&       ss,
                              GenericUnitId        left,
                              GenericUnitId        right,
                              maybe<GenericUnitId> force_top ) {
  // Sort in decreasing order of defense, then by id, but always
  // put force_top first (if specified).
  return key_func( ss, left, force_top ) >
         key_func( ss, right, force_top );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void sort_unit_stack( SSConst const&         ss,
                      vector<GenericUnitId>& units,
                      maybe<GenericUnitId>   force_top ) {
  sort( units.begin(), units.end(),
        [&]( GenericUnitId left, GenericUnitId right ) {
          return unit_stack_ordering_cmp( ss, left, right,
                                          force_top );
        } );
}

void sort_native_unit_stack( SSConst const&        ss,
                             vector<NativeUnitId>& units ) {
  sort( units.begin(), units.end(),
        [&]( NativeUnitId left, NativeUnitId right ) {
          return unit_stack_ordering_cmp(
              ss, left, right,
              /*force_top=*/nothing );
        } );
}

void sort_euro_unit_stack( SSConst const&  ss,
                           vector<UnitId>& units ) {
  sort( units.begin(), units.end(),
        [&]( UnitId left, UnitId right ) {
          return unit_stack_ordering_cmp(
              ss, left, right,
              /*force_top=*/nothing );
        } );
}

} // namespace rn
