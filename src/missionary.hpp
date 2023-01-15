/****************************************************************
**missionary.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-05.
*
* Description: All things related to missionaries.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// gs
#include "ss/unit-type.hpp"

namespace rn {

struct Colony;
struct Player;
struct Unit;

// This will determine if a colony can bless a missionary; in the
// original game this requires either a church or cathedral. Note
// that any human unit can be blessed as a missionary.
bool can_bless_missionaries( Colony const& colony );

// This will determine if the unit type can be blessed as a mis-
// sionary; in the original game, it is any human unit (once
// stripped to its base type).
bool unit_can_be_blessed( UnitType type );

// The unit does not have to be working in the colony or even in
// the colony square, though in practice the unit will always be
// one of those two.
void bless_as_missionary( Player const& player, Colony& colony,
                          Unit& unit );

// Is the unit either a missionary or jesuit missionary. A jesuit
// colonist does not count.
bool is_missionary( e_unit_type type );

} // namespace rn
