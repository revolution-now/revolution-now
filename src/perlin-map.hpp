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

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
void land_gen_perlin( PerlinMapSettings const& settings,
                      double target_density,
                      gfx::size const world_sz,
                      gfx::Matrix<e_surface>& surface );

} // namespace rn
