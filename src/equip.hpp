/****************************************************************
**equip.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-28.
*
* Description: Equipping and unequipping units from commodities.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "equip.rds.hpp"

// Revolution Now
#include "market.rds.hpp"

// ss
#include "ss/unit-id.hpp"

// C++ standard library
#include <vector>

namespace rn {

struct Colony;
struct Player;
struct SS;
struct SSConst;
struct TS;
struct Unit;
struct UnitComposition;

/****************************************************************
** Harbor
*****************************************************************/
// Will return the list of options that the unit has for equip-
// ping and unequipping while on the dock in the harbor. This as-
// sumes that there is an infinite amount of each commodity
// available (which there is, in the harbor), but it does take
// into account the player's money and the boycott status of each
// commodity. Specifically, it will return options that the
// player cannot afford, but will mark them as such, and it will
// omit any option that requires transacting in a boycotted good.
// The options will be returned such that the "equip" options
// come before the "unequip" options. Also, only options that re-
// quire one change will be returned; that means that there won't
// be a single option to transform a colonist into a dragoon (or
// vice versa) because that requires equipping with both muskets
// and horses.
std::vector<HarborEquipOption> harbor_equip_options(
    SSConst const& ss, Player const& player,
    UnitComposition const& unit_comp );

// Returns markup text with the description of the action.
std::string harbor_equip_description(
    HarborEquipOption const& option );

[[nodiscard]] PriceChange perform_harbor_equip_option(
    SS& ss, TS& ts, Player& player, UnitId unit_id,
    HarborEquipOption const& option );

/****************************************************************
** Colony
*****************************************************************/
// Given a unit this will look at the commodity store in the
// colony and return all of the possible transformations that the
// unit can undergo. Note that it will include even transitions
// that require multiple commodities, such as a colonist to a
// dragoon.
std::vector<ColonyEquipOption> colony_equip_options(
    Colony const& colony, UnitComposition const& unit_comp );

std::string colony_equip_description(
    ColonyEquipOption const& option );

void perform_colony_equip_option(
    SS& ss, TS& ts, Colony& colony, Unit& unit,
    ColonyEquipOption const& option );

} // namespace rn
