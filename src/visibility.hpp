/****************************************************************
**visibility.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-30.
*
* Description: Things related to hidden tiles and fog-of-war.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// ss
#include "ss/nation.rds.hpp"
#include "ss/unit-type.rds.hpp"

// gfx
#include "gfx/coord.hpp"

// refl
#include "refl/enum-map.hpp"

namespace rn {

struct SSConst;
struct Unit;

// Compute which nations have at least one unit that currently
// has visibility on the given tile. To qualify, the nation must
// be able to currently see the tile.
refl::enum_map<e_nation, bool> nations_with_visibility_of_square(
    SSConst const& ss, Coord tile );

// The unit's site radius is 1 for most units, two for scouts and
// ships, and then having De Soto gives a +1 to all units. In the
// OG ships don't get the De Soto bonus, but in this game we do
// give it to ships. A radius of 1 means 3x3 visibility, while a
// radius of 2 means 5x5 visibility, etc.
int unit_sight_radius( SSConst const& ss, e_nation nation,
                       e_unit_type type );

// This will look up the unit type's sighting radius, and then
// compute each (existing) square that the unit could see as-
// suming it is on the given tile, and will do so according to
// the OG's rules. This means that a land (sea) unit can see ALL
// squares in its immediate vicinity, plus all LAND (SEA) squares
// beyond that (and <= to its sighting radius). This looks a bit
// odd at first but is probably to prevent the ships with the ex-
// tended sighting radius from being able to reveal too many land
// tiles too easily, or a scout from being able to see on adja-
// cent islands (which would be easily possible after getting De
// Soto, in which case it would have 7x7 site).
std::vector<Coord> unit_visible_squares( SSConst const& ss,
                                         e_nation       nation,
                                         e_unit_type    type,
                                         Coord          tile );

} // namespace rn
