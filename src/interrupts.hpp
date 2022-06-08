/****************************************************************
**interrupts.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-08.
*
* Description: Exceptions that can be thrown to change game
*              state.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <exception>

namespace rn {

struct game_quit_interrupt : std::exception {};
struct game_load_interrupt : std::exception {};

} // namespace rn
