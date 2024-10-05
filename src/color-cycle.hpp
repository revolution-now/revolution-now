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

// refl
#include "refl/query-enum.hpp"

// C++ standard library
#include <array>

namespace rr {
struct IRenderer;
}

namespace rn {

/****************************************************************
** Forward decls.
*****************************************************************/
enum class e_color_cycle_plan;

struct IGui;

/****************************************************************
** Public API.
*****************************************************************/
void set_color_cycle_plans( rr::IRenderer& renderer );

int cycle_plan_idx( e_color_cycle_plan plan );

// A background coro thread that monitors game settings and
// evolves the color cycling animation state accordingly.
wait<> cycle_map_colors_thread(
    rr::IRenderer& renderer, IGui& gui,
    bool const& enabled ATTR_LIFETIMEBOUND );

} // namespace rn
