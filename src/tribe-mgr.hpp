/****************************************************************
**tribe-mgr.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-06.
*
* Description: High level actions to do with natives.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/dwelling-id.hpp"
#include "ss/native-enums.rds.hpp"

namespace rn {

struct SS;
struct TS;

// This is the method that normal game code should call in order
// destroy a dwelling. It is always safe to call in that it will
// perform all actions and cleanup needed to safely remove the
// dwelling from the game (along with associated braves and owned
// land). Any missionary that is in the dwelling will be elimi-
// nated. NOTE: this is an expensive call because it must iterate
// over all map squares to remove any land owned by the dwelling.
void destroy_dwelling( SS& ss, TS& ts, DwellingId dwelling_id );

// This is the method that normal game code should call in order
// destroy a tribe. It is always safe to call in that it will
// perform all actions and cleanup needed to safely remove the
// tribe from the game. NOTE: this is an expensive call to make,
// even when the tribe is small. This is because it must iterate
// over all map squares to remove any land owned by the tribe. It
// is safe to call this if the tribe does not exist, and in that
// case it will be very fast.
void destroy_tribe( SS& ss, TS& ts, e_tribe tribe );

// This will destroy the tribe and pop up a message box.
wait<> destroy_tribe_interactive( SS& ss, TS& ts,
                                  e_tribe tribe );

} // namespace rn
