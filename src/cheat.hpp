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
struct Player;
struct SS;
struct SSConst;
struct TS;
struct Unit;
struct UnitsState;

/****************************************************************
** In Land View
*****************************************************************/
// Ask the user which map they want to reveal.
wait<> cheat_reveal_map( SS& ss, TS& ts );

// No dialog box, just toggle the map view between the entire map
// and "no special view."
void cheat_toggle_reveal_full_map( SS& ss, TS& ts );

// Open a dialog box containing one check box for each founding
// father and allow the player to select/deselect.
wait<> cheat_edit_fathers( SS& ss, TS& ts, Player& player );

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
                                   Player const&  player,
                                   Unit&          unit );

void cheat_downgrade_unit_expertise( Player const& player,
                                     Unit&         unit );

void cheat_create_new_colonist( SS& ss, TS& ts,
                                Player const& player,
                                Colony const& colony );

void cheat_increase_commodity( Colony&     colony,
                               e_commodity type );
void cheat_decrease_commodity( Colony&     colony,
                               e_commodity type );

// This will perform all of the actions on the colony that are
// done when it is evolved at the start of a turn, though it
// won't display any notifications, it will just log them.
void cheat_advance_colony_one_turn( SS& ss, TS& ts,
                                    Player& player,
                                    Colony& colony );

// This is called when the player asks to just create a unit on
// the map. It will allow the player to select the unit type.
wait<> cheat_create_unit_on_map( SS& ss, TS& ts, e_nation nation,
                                 Coord tile );

/****************************************************************
** In Harbor View
*****************************************************************/
void cheat_increase_tax_rate( Player& player );

void cheat_decrease_tax_rate( Player& player );

void cheat_increase_gold( Player& player );

void cheat_decrease_gold( Player& player );

wait<> cheat_evolve_market_prices( SS& ss, TS& ts,
                                   Player& player );

void cheat_toggle_boycott( Player& player, e_commodity type );

} // namespace rn
