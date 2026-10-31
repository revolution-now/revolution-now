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
#include "gfx/matrix-fwd.hpp"

// rand
#include "rand/entropy.hpp"
#include "rand/perlin.rds.hpp"

// base
#include "base/valid.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct PerlinMapSettings;

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] rng::PerlinSeed generate_perlin_seed(
    rng::entropy e );

[[nodiscard]] base::valid_or<e_perlin_map_error> land_gen_perlin(
    PerlinMapSettings const& settings, double target_density,
    gfx::size const world_sz, gfx::matrix<e_surface>& surface );

} // namespace rn
