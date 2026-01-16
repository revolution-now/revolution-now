/****************************************************************
**perlin-hashes.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-26.
*
* Description: Hash table used for perlin noise generation.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>

namespace rng {

using PerlinHashType = uint16_t;

size_t constexpr kPerlinHashMask =
    std::numeric_limits<PerlinHashType>::max();
size_t constexpr kNumPerlinHashes =
    size_t( kPerlinHashMask ) + 1;

using PerlinHashes =
    std::array<PerlinHashType, kNumPerlinHashes>;

PerlinHashes const& perlin_hashes();

} // namespace rng
