/****************************************************************
**lumber-yield.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-20.
*
* Description: Lumber yields from pioneers clearing forests.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "lumber-yield.rds.hpp"

// ss
#include "ss/unit-type.rds.hpp"

// gfx
#include "gfx/coord.hpp"

// C++ standard library
#include <vector>

namespace rn {

struct Player;
struct SSConst;

// Will return a list of all possibly colonies in the vicinity
// that could accept some lumber yield (from clearing a forest on
// `loc`) along with how much they could receive, which could be
// zero, e.g. if the colony's capacity for lumber is filled.
std::vector<LumberYield> lumber_yields(
    SSConst const& ss, Player const& player, Coord loc,
    e_unit_type pioneer_type );

}
