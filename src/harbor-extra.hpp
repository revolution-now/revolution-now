/****************************************************************
**harbor-extra.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-02-16.
*
* Description: Helpers for the harbor view.
*
*****************************************************************/
#pragma once

// rds
#include "harbor-extra.rds.hpp"

// ss
#include "ss/unit-id.hpp"

namespace rn {

struct SSConst;

/****************************************************************
** Public API
*****************************************************************/
// This will get a list of all of the commodity slots in the
// cargo of the given unit (which is intended to be a ship in the
// harbor) and will sort them by increasing order of the sell
// value (price*quantity), which is the order in which the OG
// will auto unload (sell) them when asked to do so. It will also
// mark any boycotted commodities as such, since the player will
// need to be prompted about that.
HarborUnloadables find_unloadable_slots_in_harbor(
    SSConst const& ss, UnitId ship_id );

} // namespace rn
