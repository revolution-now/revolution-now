/****************************************************************
**terrain-mgr.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-14.
*
* Description: Helper methods for dealing with terrain.
*
*****************************************************************/
#pragma once

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/function-ref.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct MapSquare;
struct RealTerrain;
struct SSConst;

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] int num_surrounding_land_tiles( SSConst const& ss,
                                              gfx::point tile );

[[nodiscard]] int num_surrounding_land_tiles(
    RealTerrain const& terrain, gfx::point tile );

[[nodiscard]] bool is_island( SSConst const& ss,
                              gfx::point tile );

[[nodiscard]] bool is_island( RealTerrain const& terrain,
                              gfx::point tile );

void on_all_tiles( SSConst const& ss,
                   base::function_ref<void(
                       gfx::point, MapSquare const& square )>
                       fn );

} // namespace rn
