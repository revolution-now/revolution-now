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

std::string construction_name(
    Construction_t const& construction );

// The outter maybe is when the user just escapes, the inner one
// is for when they select no production.
wait<> select_colony_construction( Colony& colony, IGui& gui );

} // namespace rn
