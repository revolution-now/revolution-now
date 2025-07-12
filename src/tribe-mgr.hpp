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

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/dwelling-id.hpp"

namespace rn {

struct Dwelling;
struct IEuroAgent;
struct IMapUpdater;
struct SS;
struct SSConst;

enum class e_tribe;

/****************************************************************
** Destroying things.
*****************************************************************/
// This is the method that normal game code should call in order
// destroy a dwelling. It is always safe to call in that it will
// perform all actions and cleanup needed to safely remove the
// dwelling from the game (along with associated braves and owned
// land). Any missionary that is in the dwelling will be elimi-
// nated. NOTE: this is an expensive call because it must iterate
// over all map squares to remove any land owned by the dwelling.
void destroy_dwelling( SS& ss, IMapUpdater& map_updater,
                       DwellingId dwelling_id );

// This is the method that normal game code should call in order
// destroy a tribe. It is always safe to call in that it will
// perform all actions and cleanup needed to safely remove the
// tribe from the game. NOTE: this is an expensive call to make,
// even when the tribe is small. This is because it must iterate
// over all map squares to remove any land owned by the tribe. It
// is safe to call this if the tribe does not exist, and in that
// case it will be very fast.
void destroy_tribe( SS& ss, IMapUpdater& map_updater,
                    e_tribe tribe );

// This will destroy the tribe and pop up a message box.
wait<> destroy_tribe_interactive( SS& ss, IEuroAgent& agent,
                                  IMapUpdater& map_updater,
                                  e_tribe tribe );

// Displays the message that a tribe is gone.
wait<> tribe_wiped_out_message( IEuroAgent& agent,
                                e_tribe tribe );

// This is the way to get the tribe for a dwelling that will work
// for both real dwelling and frozen dwellings that may no longer
// exist. Generally, whenever we are dealing with dwellings on
// the map that may not be directly adjacent to the player's
// units, we need to make sure that what we're doing works for
// both real dwellings and phantom ones.
e_tribe tribe_type_for_dwelling( SSConst const& ss,
                                 Dwelling const& dwelling );

} // namespace rn
