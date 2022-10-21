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

// Given some yields, find the "best" one in which to deposit the
// lumber yield. This will simply choose the one which can actu-
// ally receive the most. If two yields are considered equivalent
// then the one that is earlier in the list will be chosen. If a
// yield is returned the it is guaranteed to have non-zero actual
// depositable yield into the colony.
base::maybe<LumberYield> best_lumber_yield(
    std::vector<LumberYield> const& yields );

} // namespace rn
