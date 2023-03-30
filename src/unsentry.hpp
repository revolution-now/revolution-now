/****************************************************************
**unsentry.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-26.
*
* Description: Handles logic related to unsentrying sentried
*              units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// ss
#include "ss/nation.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct SS;
struct Unit;

// For each of the given nation's units that are directly on the
// map, it will unsentry them if they are sentried and if they
// are adjacent to a foreign unit.
void unsentry_units_next_to_foreign_units(
    SS& ss, e_nation nation_to_unsentry );

// Given a euro unit that is on the map, this will unsenty any
// foreign sentried units immediately adjacent to it.
void unsentry_foreign_units_next_to_euro_unit(
    SS& ss, Unit const& unit );

// Given the coordinate of a unit, this will unsenty any euro
// sentried units immediately adjacent to it. This is used to un-
// sentry euro units around native units, in which case the
// tribe/type of the native unit is irrelevant.
void unsentry_units_next_to_tile( SS& ss, Coord coord );

} // namespace rn
