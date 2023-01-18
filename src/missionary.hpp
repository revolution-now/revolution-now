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

/****************************************************************
** Note on convert expiration.
*****************************************************************/
// Note: In the OG, if a native convert is not put to work within
// eight turns then the unit disappears. However, in this game we
// don't replicate that, since it seems that it was originally
// added to incentivize the player to use the native convert be-
// fore it was made stronger at outdoor jobs than free colonists.
// From the strategy guide:
//
//   "The eight-tum rule is a vestige ef an earlier version ef
//   the game in which Native Converts were not supen'or to Free
//   Colonists in wry capacity but retained the disadvantages
//   they possess now. In that version, it was sometimes diffi-
//   cult to find a use for Native Converts because Free
//   Colonists and other colonist types were usually equal or su-
//   pen·or to them. The bonusfor Native Convert production was a
//   late addition to the game, which now makes them attractive
//   colonists. It also increases the value efcreating Mission-
//   aries and establishing missions."

/****************************************************************
** Public API
*****************************************************************/
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
