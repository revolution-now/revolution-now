/****************************************************************
**road.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-25.
*
* Description: Road rendering and state changes.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "game-state.hpp"
#include "unit.hpp"

// render
#include "render/fwd.hpp"

namespace rn {

/****************************************************************
** Road State
*****************************************************************/
// Must be a land square or check fail.
void set_road( TerrainState& terrain_state, Coord tile );
void clear_road( TerrainState& terrain_state, Coord tile );
bool has_road( TerrainState const& terrain_state, Coord tile );

/****************************************************************
** Unit State
*****************************************************************/
// Will advance the state of a unit that is building a road. If
// the unit is finished building the road it will clear that
// unit's orders, subtract some of its tools, and create the
// road. Otherwise, it will increase the units number of turns
// worked and consume the unit's movement points. The unit must
// have road-building orders in order to call this method, and
// you will know that the unit finished building the road when
// its orders are cleared. If the unit has the remainder of its
// tools removed by this function then the unit will be demoted.
void perform_road_work( UnitsState const& units_state,
                        TerrainState&     terrain_state,
                        Unit&             unit );

bool can_build_road( Unit const& unit );

/****************************************************************
** Rendering
*****************************************************************/
void render_road_if_present( rr::Painter& painter, Coord where,
                             TerrainState const& terrain_state,
                             Coord               world_tile );

} // namespace rn
