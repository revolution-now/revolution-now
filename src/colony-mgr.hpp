/****************************************************************
**colony-mgr.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-01.
*
* Description: Main interface for controlling Colonies.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "colony-id.hpp"
#include "coord.hpp"
#include "error.hpp"
#include "expect.hpp"
#include "nation.hpp"
#include "unit-id.hpp"
#include "wait.hpp"

// Rds
#include "colony-mgr.rds.hpp"

// C++ standard library
#include <string_view>

namespace rn {

valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    std::string_view name );

valid_or<e_found_colony_err> unit_can_found_colony(
    UnitId founder );

// This will change the nation of the colony and all units that
// are workers in the colony as well as units that are in the
// same map square as the colony.
void change_colony_nation( ColonyId id, e_nation new_nation );

// Before calling this, it should already have been the case that
// `can_found_colony` was called to validate; so it should work,
// and thus if it doesn't, it will check-fail.
ColonyId found_colony_unsafe( UnitId           founder,
                              std::string_view name );

// Evolve the colony by one turn.
wait<> evolve_colony_one_turn( ColonyId id );

} // namespace rn
