/****************************************************************
**perlin-map.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-10.
*
* Description: Perlin Noise based map generation routines.
*
*****************************************************************/
#pragma once

// rds
#include "perlin-map.rds.hpp"

// ss
#include "ss/terrain-enums.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/matrix.hpp"

// base
#include "base/valid.hpp"

namespace rng {
struct entropy;
}

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] PerlinSeed generate_perlin_seed(
    rng::entropy entropy );

[[nodiscard]] base::valid_or<e_perlin_map_error> land_gen_perlin(
    PerlinMapSettings const& settings, double target_density,
    gfx::size const world_sz, gfx::Matrix<e_surface>& surface );

} // namespace rn
