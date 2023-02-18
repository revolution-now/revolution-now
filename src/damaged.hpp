/****************************************************************
**damaged.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-17.
*
* Description: Things related to damaged ships.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// rds
#include "damaged.rds.hpp"

// ss
#include "ss/nation.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct SSConst;

// Given a ship that is assumed damaged at the given map loca-
// tion, this will find a place for it to be repaired. First it
// will look for any friendly colonies with either Drydocks or
// Shipyards and it will choose the closest one. Otherwise it
// will select the european harbor.
ShipRepairPort_t find_repair_port_for_ship(
    SSConst const& ss, e_nation nation, Coord ship_location );

} // namespace rn
