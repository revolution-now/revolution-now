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
#include "waitable.hpp"

// C++ standard library
#include <exception>

namespace rn {

struct game_quit_exception : std::exception {};

waitable<> next_turn();

} // namespace rn
