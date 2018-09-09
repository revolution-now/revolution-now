/****************************************************************
* turn.hpp
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

namespace rn {

enum class ND e_turn_result {
  cont,
  quit
};

// Do a turn, start to finish.
ND e_turn_result turn();

} // namespace rn

