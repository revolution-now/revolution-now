/****************************************************************
**construction.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-20.
*
* Description: All things related to colony construction.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"
#include "wait.hpp"

// gs
#include "ss/colony.rds.hpp"

// C++ standard library
#include <string>

namespace rn {

struct IGui;
struct Colony;
struct SS;
struct SSConst;

std::string construction_name(
    Construction_t const& construction );

// The outter maybe is when the user just escapes, the inner one
// is for when they select no production.
wait<> select_colony_construction( SSConst const& ss,
                                   Colony& colony, IGui& gui );

// Computes the cost of rushing the construction project given
// the hammers and tools already in the colony. There must be an
// active construction project in the colony otherwise this will
// check-fail.
int rush_construction_cost( SSConst const& ss,
                            Colony const&  colony );

// Opens the box that opens when the user asks to rush completion
// of a construction project. There must be an active construc-
// tion project in the colony otherwise this will check-fail. The
// cost could be computed from the other parameters, but instead
// we just compute it in a separate function and pass it in to
// make things more easily testable. If `allow_tools_boycott` is
// false then the player will be prevented from rushing the con-
// struction if it requires acquiring tools while tools are under
// boycott.
wait<> rush_construction_prompt( SS& ss, Colony& colony,
                                 IGui& gui, int cost_to_charge,
                                 bool allow_tools_boycott );

} // namespace rn
