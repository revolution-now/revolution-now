/****************************************************************
**perlin.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-24.
*
* Description: Perlin noise generator.
*
*****************************************************************/
#pragma once

// rng
#include "vec.hpp"

// C++ standard library
#include <cstdint>

namespace rng {

/****************************************************************
** Types.
*****************************************************************/
using PerlinInt   = uint32_t;
using PerlinFloat = double;
using PerlinVec2  = vec2;

struct PerlinFractalOptions {
  int const n_octaves           = 1;
  PerlinFloat const persistence = 0.5;
  PerlinFloat const lacunarity  = 2.0;
};

/****************************************************************
** Perlin Noise.
*****************************************************************/
// Returns a value between [-1, 1].
[[nodiscard]] PerlinFloat perlin_noise_2d(
    PerlinVec2 const point,
    PerlinFractalOptions const& fractal_options,
    // Repeat doesn't just repeat the pattern (which could be
    // done manually by the caller); instead it causes it to re-
    // peat without seams, meaning that it will tile continu-
    // ously. This is because the repeat is applied not just to
    // the sample coordinates but also to the gradients on the
    // grid which, together with the smoothing function used, en-
    // sures that both the value and gradient are continuous at
    // repeat boundaries. Thus, it must be exposed as a parameter
    // to this function instead of the caller doing manual tiling
    // of a subsection of the result.
    PerlinVec2 const seamless_repeat,
    // Repeats every #hashes, but does not merely translate or
    // shift the map because different octaves get shifted by the
    // same absolute amount which changes their position relative
    // to each other because they have different frequencies.
    // This is almost kind of like a seed.
    PerlinInt const base );

} // namespace rng
