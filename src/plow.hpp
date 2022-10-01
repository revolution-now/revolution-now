/****************************************************************
**plow.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-27.
*
* Description: Plowing rendering and state changes.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "unit.hpp"

// render
#include "render/fwd.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct IMapUpdater;
struct MapSquare;
struct TerrainState;
struct UnitsState;

/****************************************************************
** Plow State
*****************************************************************/
// Must be a land square or check fail. If this is a forest then
// it will clear it, otherwise it will add irrigation. If neither
// of those are possible then it will check-fail, so you should
// call can_plow on this square first.
void plow_square( TerrainState const& terrain_state,
                  IMapUpdater& map_updater, Coord tile );

// Can we either clear a forest on the square or add irrigation.
bool can_plow( TerrainState const& terrain_state, Coord tile );

// This applies only to irrigation, not clearing.
bool can_irrigate( TerrainState const& terrain_state,
                   Coord               tile );

// This includes *only* irrigation and not forest-clearing. Will
// return false if the square already has irrigation.
bool can_irrigate( MapSquare const& square );

bool has_irrigation( TerrainState const& terrain_state,
                     Coord               tile );

bool has_irrigation( MapSquare const& square );

/****************************************************************
** Unit State
*****************************************************************/
// Will advance the state of a unit that is plowing. If the unit
// is finished the plowing it will clear that unit's orders, sub-
// tract some of its tools, and create the plowing's result. Oth-
// erwise, it will increase the units number of turns worked and
// consume the unit's movement points. The unit must have plowing
// orders in order to call this method, and you will know that
// the unit finished plowing when its orders are cleared. If the
// unit has the remainder of its tools removed by this function
// then the unit will be demoted.
void perform_plow_work( UnitsState const&   units_state,
                        TerrainState const& terrain_state,
                        IMapUpdater& map_updater, Unit& unit );

bool can_plow( Unit const& unit );

/****************************************************************
** Rendering
*****************************************************************/
void render_plow_if_present( rr::Painter& painter, Coord where,
                             MapSquare const& square );

} // namespace rn
