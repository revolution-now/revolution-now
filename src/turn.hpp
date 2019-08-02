/****************************************************************
**turn.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Main loop that processes a turn.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "nation.hpp"

namespace rn {

enum class ND e_turn_result {
  orders_taken,
  no_orders_taken,
  quit
};

// Do a turn, start to finish, one nations.
ND e_turn_result turn( e_nation nation );

// Do a turn, start to finish, all nations.
ND e_turn_result turn();

void linker_dont_discard_module_turn();

} // namespace rn
