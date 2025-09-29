/****************************************************************
**pacific.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-28.
*
* Description: Helpers for distinguishing pacific/atlantic.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct TerrainState;

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] bool is_atlantic_side_of_map(
    TerrainState const& terrain, gfx::point tile );

[[nodiscard]] bool is_pacific_side_of_map(
    TerrainState const& terrain, gfx::point tile );

} // namespace rn
