/****************************************************************
**on-map.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-18.
*
* Description: Handles actions that need to be take in response
*              to a unit appearing on a map square (after
*              creation or moving).
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "unit-id.hpp"
#include "wait.hpp"

// ss
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct SS;
struct TS;

// A dummy type to help remind the caller that the unit may have
// disappeared as a result of the call. This works because maybe
// types are [[nodiscard]].
struct UnitDeleted {};

// Whenever a unit is placed on a map square for any reason
// (whether they moved there, were created there, appeared there,
// etc.) this must be called to perform the correct game updates
// (and that includes moving the unit to the target square, which
// this function will do).
//
// WARNING: After this function completes, the unit may no longer
// exist since they might stepped into a lost city rumor and dis-
// appeared! Or new units could have been created, etc.
wait<maybe<UnitDeleted>> unit_to_map_square(
    SS& ss, TS& ts, UnitId id, Coord world_square );

// This is the non-coroutine version of the above, only to be
// called from non-coroutines where you know that this action
// won't need to trigger any UI actions.
void unit_to_map_square_non_interactive( SS& ss, TS& ts,
                                         UnitId id,
                                         Coord  world_square );
void unit_to_map_square_non_interactive( SS& ss, NativeUnitId id,
                                         Coord world_square );

} // namespace rn
