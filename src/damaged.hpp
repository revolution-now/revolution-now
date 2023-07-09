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

// Revolution Now
#include "maybe.hpp"

// ss
#include "ss/nation.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct SSConst;
struct SS;
struct TS;
struct Unit;

enum class e_unit_type;

// Given a ship that is assumed damaged at the given map loca-
// tion, this will find a place for it to be repaired. First it
// will look for any friendly colonies with either Drydocks or
// Shipyards and it will choose the closest one. Otherwise it
// will select the european harbor.
//
// However, if independence has been declared then the european
// harbor cannot be selected; hence, if it would have otherwise
// chosen that, then instead it will return nothing. This is be-
// cause ships cannot go to europe to be repaired in that case.
maybe<ShipRepairPort> find_repair_port_for_ship(
    SSConst const& ss, e_nation nation, Coord ship_location );

// Produce the standard message that should be displayed to the
// user when they try to interact with a damaged ship in a way
// that is not allowed.
std::string ship_still_damaged_message(
    int turns_until_repaired );

// Produces the message that is displayed when a ship is damaged.
std::string ship_damaged_message( SSConst const&        ss,
                                  Unit const&           ship,
                                  ShipRepairPort const& port );

// This one generates a message for the case where a ship is dam-
// aged but there is no port to send it to. This would only
// happen after independence is declared, after which the euro-
// pean harbor is no longer accessible.
std::string ship_damaged_no_port_message( Unit const& ship );

// Generates a message describing how many units onboard a ship
// have been lost, if any.
maybe<std::string> units_lost_on_ship_message(
    Unit const& ship );

// Mark the ship as damaged, destroy any units it contains, and
// move it to where it needs to go for repairs.
void move_damaged_ship_for_repair( SS& ss, TS& ts, Unit& ship,
                                   ShipRepairPort const& port );

// When a ship is damaged this will return the number of turns
// that it will need until repaired. Note that this could be zero
// (in the OG that would be a Caravel in a drydock), in which
// case the unit is not damaged but instead is immediately ready
// that same turn, and can move if it hasn't yet.
int repair_turn_count_for_unit( ShipRepairPort const& port,
                                e_unit_type           type );

} // namespace rn
