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

// Rds
#include "plow.rds.hpp"

// Revolution Now
#include "unit.hpp"

// render
#include "render/fwd.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct IMapUpdater;
struct MapSquare;
struct Player;
struct SS;
struct SSConst;
struct TerrainState;

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

// Is there already a pioneer working on this tile?
bool has_pioneer_working( SSConst const& ss, Coord tile );

/****************************************************************
** Unit State
*****************************************************************/
// Will advance the state of a unit that is plowing. If the unit
// is finished the plowing it will clear that unit's orders, sub-
// tract some of its tools, and create the plowing's result. Oth-
// erwise, it will increase the units number of turns worked and
// consume the unit's movement points. The unit must have plowing
// orders in order to call this method. If the unit has the re-
// mainder of its tools removed by this function then the unit
// will be demoted.
//
// Note that if this function call results in a forest getting
// cleared then it will compute the lumber yield and the colony
// in which to place it (if any) and will add the lumber to the
// colony's stockpile. It will return the lumber yield info in
// the result so that a message can be displayed to the user.
[[nodiscard]] PlowResult_t perform_plow_work(
    SS& ss, Player const& player, IMapUpdater& map_updater,
    Unit& unit );

bool can_plow( Unit const& unit );

/****************************************************************
** Rendering
*****************************************************************/
void render_plow_if_present( rr::Painter& painter, Coord where,
                             MapSquare const& square );

} // namespace rn
