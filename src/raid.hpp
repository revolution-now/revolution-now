/****************************************************************
**raid.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-10.
*
* Description: Orchestrates the process of a brave raiding
*              a european unit or colony.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "wait.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct Colony;
struct NativeUnit;
struct SS;
struct TS;

// Brave attacks a european unit outside of a colony.
wait<> raid_unit( SS& ss, TS& ts, NativeUnit& attacker,
                  Coord dst );

// Brave attacks a colony.
wait<> raid_colony( SS& ss, TS& ts, NativeUnit& attacker,
                    Colony& colony );

} // namespace rn
