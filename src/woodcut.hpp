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

struct IEuroMind;
struct IGui;
struct Player;

// Displays a "woodcut" if it has not yet been displayed. If it
// gets displayed then it will mark it as having been displayed
// so that it doesn't get displayed again. This will actually
// display the woodcut not directly, but instead via the corre-
// sponding IGui interface so that it can be mocked.
wait<> show_woodcut_if_needed( Player& player, IEuroMind& mind,
                               e_woodcut cut );

namespace internal {
// Displays a "woodcut." This is a kind of "cut scene" consisting
// of a screen-filling image that is enpixelated over a back-
// ground of wood textures (hence "woodcut") in response to a
// one-time event that happens in the game. Note that this method
// should not be called directly by most game code; instead the
// method in the IEuroMind should be called.
wait<> show_woodcut( IGui& gui, e_woodcut cut );
}

} // namespace rn
