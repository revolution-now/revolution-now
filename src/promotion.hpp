/****************************************************************
**promotion.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-10.
*
* Description: All things related to unit type promotion.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace rn {

struct SSConst;
struct Unit;

// This is the function that top-level game logic should call
// whenever it is determined that a unit is up for promotion (the
// it is ok if the unit cannot be promoted further). This will
// find the unit's current activity and promote it along those
// lines if a promotion target exists. It will return true if a
// promotion was possible (and was thus made). If the unit has no
// activity the no promotion will be made. Technically, in that
// case, we could still theoretically promote e.g. an indentured
// servant to a free colonist, but the game will never do that
// (outside of cheat mode) when the unit has no activity, so we
// don't do that. Also, note that if the unit is an expert at
// something other than the given activity then no promotion will
// happen.
//
// TODO: promote veterans to continentals after independence.
//
bool try_promote_unit_for_current_activity( SSConst const& ss,
                                            Unit& unit );

} // namespace rn
