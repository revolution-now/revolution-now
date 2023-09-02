/****************************************************************
**map-search.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-20.
*
* Description: Algorithms for searching the map.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "maybe.hpp"

// ss
#include "ss/colony-id.hpp"
#include "ss/nation.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"

// C++ standard library
#include <vector>

namespace rn {

struct Colony;
struct FogColony;
struct Player;
struct SSConst;

// Returns a list of existing map squares spiraling outward from
// the starting point and which are within `max_distance`
// pythagorean distance from the start.
std::vector<gfx::point> outward_spiral_pythdist_search_existing(
    SSConst const ss, gfx::point const start,
    double max_distance );

// Starting from (and including) `location`, find the square con-
// taining any colony that is either friendly or which has been
// found through exploration (that is, it is currently either
// visible or fogged) that is the shortest pythagorean distance
// away, among those whose distance is less than `max_distance`.
// Note that this returns a FogColony, and so said colony may no
// longer exist; in fact, even the nation may no longer exist.
maybe<FogColony const&> find_close_explored_colony(
    SSConst const& ss, e_nation nation, gfx::point location,
    double max_distance );

// Returns a list of friendly colonies spiraling outward from the
// starting point that are within a radius of `max_distance` to
// the start.
std::vector<ColonyId> close_friendly_colonies(
    SSConst const& ss, Player const& player,
    gfx::point const start, double max_distance );

} // namespace rn
