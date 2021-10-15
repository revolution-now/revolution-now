/****************************************************************
**game-state.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-02.
*
* Description: Holds the serializable state of a game.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// FIXME: once the save-game state is consoldiated it should be
// handled in this module, but until then, this module will only
// have some of the serializable game state.

namespace rn {

bool is_independence_declared();

} // namespace rn
