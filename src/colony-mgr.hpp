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

enum class ND e_found_colony_result {
  good,
  colony_exists_here,
  no_water_colony,
  colonist_not_on_map,
  ship_cannot_found_colony
};

e_found_colony_result can_found_colony( UnitId founder );

expect<ColonyId> found_colony( UnitId           founder,
                               std::string_view name );

} // namespace rn
