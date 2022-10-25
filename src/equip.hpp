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

struct Player;
struct SS;
struct SSConst;
struct UnitComposition;

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
    SS& ss, Player& player, UnitId unit_id,
    HarborEquipOption const& option );

} // namespace rn
