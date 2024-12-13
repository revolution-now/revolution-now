/****************************************************************
**unit-transformation.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-04.
*
* Description: Represents unit type and inventory, along with
*              type transformations resulting from inventory
*              changes.
*
*****************************************************************/
#pragma once

// Rds
#include "unit-transformation.rds.hpp"

// ss
#include "ss/unit.hpp"

// C++ standard library
#include <vector>

namespace rn {

struct SS;

/****************************************************************
** Transformations
*****************************************************************/
// What derived/base units can this unit become, along with the
// delta of commodities and modifiers required for each one. The
// resulting commodity deltas are interpreted as applying to the
// provided commodity store which is assumed to be the one that
// will be used for supplying commodities for the unit change.
// Also, commodities that the unit already has will be reused.
// For example, if a soldier is transformed into a dragoon, the
// muskets will be reused, so the only commodity delta will be
// -50 horses, meaning that 50 horses would have to be subtracted
// from the commodity store in order to affect that transforma-
// tion. Also in that example, there would be one modifier delta,
// namely adding `horses`. Finally, note that this function will
// filter out any results that would result in any commodity in
// the commodity store having a negative quantity (i.e., there is
// not enough). In the cases where a unit transformation could
// potentially consume a variable number of commodities (e.g.
// tools for a pioneer) this function will assume that we want to
// use the maximum number allowed by the unit and that is avail-
// able in the commodity store.
std::vector<UnitTransformation> possible_unit_transformations(
    UnitComposition const& comp,
    std::unordered_map<e_commodity, int> const&
        commodity_store );

// Test if one can be converted to another through a valid path
// using only the commodities available from the source unit. If
// a valid path is found then return the transformation details.
maybe<UnitTransformation> query_unit_transformation(
    UnitComposition const& from_comp,
    UnitComposition const& to_comp );

// Strip a unit of all commodities and modifiers and return the
// single base type along with the commodity and modifier deltas
// that it took to get there. The commodity deltas will always be
// positive, meaning that the unit is shedding commodities.
UnitTransformation strip_to_base_type(
    UnitComposition const& comp );

// Can unit receive commodity, and if so how many and what unit
// type will it become?
std::vector<UnitTransformationFromCommodity>
unit_receive_commodity( UnitComposition const& comp,
                        Commodity const& commodity );

// Can a commodity be taken from the unit, and if so what unit
// type will it become?
std::vector<UnitTransformationFromCommodity> unit_lose_commodity(
    UnitComposition const& comp, Commodity const& commodity );

// Can unit receive commodity, and if so how many and what unit
// type will it become? This will not mutate the unit in any way.
// If you want to affect the change, then you have to look at the
// results, pick one that you want, and then call change_type
// with the UnitComposition that it contains.
std::vector<UnitTransformationFromCommodity>
with_commodity_added( Unit const& unit,
                      Commodity const& commodity );

// Similar to the above, but removing.
std::vector<UnitTransformationFromCommodity>
with_commodity_removed( Unit const& unit,
                        Commodity const& commodity );

// The unit must have at least 20 tools, which will be sub-
// tracted. If the unit ends up with zero tools then the type
// will be demoted.
void consume_20_tools( SS& ss, TS& ts, Unit& unit );

void adjust_for_independence_status(
    std::vector<UnitTransformation>& input,
    bool independence_declared );

void adjust_for_independence_status(
    std::vector<UnitTransformationFromCommodity>& input,
    bool independence_declared );

} // namespace rn
