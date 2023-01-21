/****************************************************************
**woodcut.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-21.
*
* Description: Displays woodcuts (cut scene images from the OG).
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/woodcut.rds.hpp"

namespace rn {

struct IGui;
struct Player;
struct TS;

namespace detail {
// Displays a "woodcut." This is a kind of "cut scene" consisting
// of a screen-filling image that is enpixelated over a back-
// ground of wood textures (hence "woodcut") in response to a
// one-time event that happens in the game. Note that this method
// should not be called directly by most game code; instead the
// one below should be called. This one should only be called by
// the real IGui implementation.
wait<> display_woodcut( IGui& gui, e_woodcut cut );
}

// Displays a "woodcut" if it has not yet been displayed. If it
// gets displayed then it will mark it as having been displayed
// so that it doesn't get displayed again. This will actually
// display the woodcut not directly, but instead via the corre-
// sponding IGui interface so that it can be mocked.
wait<> display_woodcut_if_needed( TS& ts, Player& player,
                                  e_woodcut cut );

} // namespace rn
