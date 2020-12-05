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
#include "coord.hpp"
#include "errors.hpp"
#include "id.hpp"
#include "nation.hpp"

// C++ standard library
#include <string_view>

namespace rn {

expect<> check_colony_invariants_safe( ColonyId id );
void     check_colony_invariants_die( ColonyId id );

enum class ND e_found_colony_err {
  colony_exists_here,
  no_water_colony,
  colonist_not_on_map,
  ship_cannot_found_colony
};

enum class e_new_colony_name_err {
  already_exists,
  name_too_short
};

valid_or<e_new_colony_name_err> is_valid_new_colony_name(
    std::string_view name );

valid_or<e_found_colony_err> unit_can_found_colony(
    UnitId founder );

// Before collaing this, it should already have been the case
// that `can_found_colony` was called to validate; so it should
// work, and thus if it doesn't, it will check-fail.
ColonyId found_colony_unsafe( UnitId           founder,
                              std::string_view name );

// Evolve the colony by one turn.
void evolve_colony_one_turn( ColonyId id );

} // namespace rn
