/****************************************************************
**cheat.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-16.
*
* Description: Implements cheat mode.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "cheat.rds.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/commodity.rds.hpp"
#include "ss/nation.rds.hpp"
#include "ss/unit-id.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct ColoniesState;
struct Colony;
struct IGui;
struct IMapUpdater;
struct SS;
struct SSConst;
struct TS;
struct Unit;
struct UnitsState;

/****************************************************************
** In Colony View
*****************************************************************/
// Allows adding and removing buildings individually or in bulk.
wait<> cheat_colony_buildings( Colony& colony, IGui& gui );

// Allows changing a unit's type based on what it's doing, if
// anything. The unit's activity wil be queried, and it will be
// upgraded based on that. Petty criminal always goes to inden-
// tured servant, which goes to free colonist. A free colonist
// will be upgraded to an expert if there is a relevant activity.
// An expert with no activity won't be changed, but an expert
// with a different activity will be switched. If the unit is a
// derived type then it will attempt to promote it without
// changing its occupation. This is to replicate the original
// game's cheat feature where you can select a unit (at least in
// the colony view) and it will be upgraded based on what it is
// currently doing or being.
void cheat_upgrade_unit_expertise( SSConst const& ss,
                                   Unit&          unit );

void cheat_downgrade_unit_expertise( Unit& unit );

void cheat_create_new_colonist( SS& ss, TS& ts,
                                Colony const& colony );

void cheat_increase_commodity( Colony&     colony,
                               e_commodity type );
void cheat_decrease_commodity( Colony&     colony,
                               e_commodity type );

// This will perform all of the actions on the colony that are
// done when it is evolved at the start of a turn, though it
// won't display any notifications, it will just log them.
void cheat_advance_colony_one_turn( SS& ss, TS& ts,
                                    Colony& colony );

// This is called when the player asks to just create a unit on
// the map. It will allow the player to select the unit type.
wait<> cheat_create_unit_on_map( SS& ss, TS& ts, e_nation nation,
                                 Coord tile );

} // namespace rn
