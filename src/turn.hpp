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

namespace rn {

enum class e_turn_result {
  cont,
  quit
};

// Do a turn, start to finish.
e_turn_result turn();

} // namespace rn

