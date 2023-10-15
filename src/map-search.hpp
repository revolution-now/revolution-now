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

// rds
#include "map-search.rds.hpp"

// Revolution Now
#include "maybe.hpp"

// ss
#include "ss/colony-id.hpp"
#include "ss/nation.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/function-ref.hpp"

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
    SSConst const& ss, gfx::point const start,
    double max_distance );

// Find the closest colony within a pythagorean distance of
// `max_distance` that satisfies the predicate. Note that this
// function itself does not check visibility or ownership of the
// colonies in any way; if needed, that should be done in the
// predicate.
maybe<Colony const&> find_any_close_colony(
    SSConst const& ss, gfx::point location, double max_distance,
    base::function_ref<bool( Colony const& )> pred );

// Starting from (and including) `location`, find the square con-
// taining any colony that is either friendly or which has been
// found through exploration (that is, it is currently either
// visible or fogged) that is the shortest pythagorean distance
// away, among those whose distance is less than `max_distance`.
//
// We must return an ExploredColony type here because the
// "colony" that is found might be either a real colony (Colony)
// or a FogColony (explored but not visible and in fact may no
// longer exist), and those don't really have a common base type.
// So the ExploredColony type contains only the info that is
// common to both that the current callers need to know about.
maybe<ExploredColony> find_close_explored_colony(
    SSConst const& ss, e_nation nation, gfx::point location,
    double max_distance );

// Returns a list of friendly colonies spiraling outward from the
// starting point that are within a radius of `max_distance` to
// the start.
std::vector<ColonyId> close_friendly_colonies(
    SSConst const& ss, Player const& player,
    gfx::point const start, double max_distance );

} // namespace rn
