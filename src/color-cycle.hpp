/****************************************************************
**color-cycle.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-04.
*
* Description: Game-specific color-cycling logic.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "wait.hpp"

namespace rr {
struct IRenderer;
}

namespace rn {

struct IGui;

// A background coro thread that monitors game settings and
// evolves the color cycling animation state accordingly.
wait<> cycle_map_colors_thread(
    rr::IRenderer& renderer, IGui& gui,
    bool const& enabled ATTR_LIFETIMEBOUND );

} // namespace rn
