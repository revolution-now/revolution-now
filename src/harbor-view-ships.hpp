/****************************************************************
**harbor-view-ships.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-02-06.
*
* Description: Common things for the views in the harbor that
*              contain ships.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"

enum class e_unit_type;

namespace rr {
struct Renderer;
}

namespace rn {

void render_unit_glow( rr::Renderer& renderer, gfx::point where,
                       e_unit_type type, int downsample_factor );

} // namespace rn
