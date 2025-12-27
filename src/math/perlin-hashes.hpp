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

namespace math {

size_t constexpr kNumPerlinHashes = 65536;
size_t constexpr kPerlinHashMask  = 65535;

using PerlinHashes = std::array<uint16_t, kNumPerlinHashes>;

PerlinHashes const& perlin_hashes();

} // namespace math
