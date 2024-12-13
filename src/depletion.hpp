/****************************************************************
**depletion.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-29.
*
* Description: Handles depletion of prime resources.
*
*****************************************************************/
#pragma once

// Rds
#include "depletion.rds.hpp"

// C++ standard library
#include <vector>

namespace rn {

struct Colony;
struct IMapUpdater;
struct IRand;
struct MapSquare;
struct SS;
struct SSConst;

/****************************************************************
** Public API
*****************************************************************/
// This is called once per turn per colony when evolving the
// colony in order to track prime resource mining and depletion
// thereof. This will mutate the depletion counters state but
// will not make any changes to tiles. This call should be fol-
// lowed up by a call to update_depleted_tiles.
[[nodiscard]] std::vector<DepletionEvent>
advance_depletion_state( SS& ss, IRand& rand,
                         Colony const& colony );

// Makes tile changes in response to depletion events.
void update_depleted_tiles(
    IMapUpdater& map_updater,
    std::vector<DepletionEvent> const& events );

// Remove the depletion counter for the tile if the tile has con-
// tents that make it no longer relevant. This should be called
// whenever a single terrian square is mutated. If the entire map
// is mutated then call the variant below.
void remove_depletion_counter_if_needed( SS& ss, Coord tile );

// Same as above but over the entire map. This is an expensive
// function, so should only be used when the entire map is up-
// dated, e.g. by the map editor.
void remove_depletion_counters_where_needed( SS& ss );

} // namespace rn
