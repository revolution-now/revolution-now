/****************************************************************
**on-map.cpp
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
#include "on-map.hpp"

// Revolution Now
#include "gs-units.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
void unit_to_map_square( UnitsState& units_state, IMapUpdater&,
                         UnitId id, Coord world_square ) {
  // 1. Move the unit. This is the only place where this function
  // should be called by normal game code.
  units_state.change_to_map( id, world_square );

  // 2. Unsentry surrounding foreign units.
  // TODO

  // 3. Update terrain visibility.
  // TODO
}

} // namespace rn
