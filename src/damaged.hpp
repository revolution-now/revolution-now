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
struct TS;

enum class e_unit_type;

// Given a ship that is assumed damaged at the given map loca-
// tion, this will find a place for it to be repaired. First it
// will look for any friendly colonies with either Drydocks or
// Shipyards and it will choose the closest one. Otherwise it
// will select the european harbor.
ShipRepairPort_t find_repair_port_for_ship(
    SSConst const& ss, e_nation nation, Coord ship_location );

// Produce the standard message that should be displayed to the
// user when they try to interact with a damaged ship in a way
// that is not allowed.
std::string damaged_ship_message( int turns_until_repaired );

// When a ship is damaged this will return the number of turns
// that it will need until repaired. Note that this could be zero
// (in the OG that would be a Caravel in a drydock), in which
// case the unit is not damaged but instead is immediately ready
// that same turn, and can move if it hasn't yet.
int repair_turn_count_for_unit( ShipRepairPort_t const& port,
                                e_unit_type             type );

} // namespace rn
