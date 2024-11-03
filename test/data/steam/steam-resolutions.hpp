/****************************************************************
**steam-resolutions.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-11-01.
*
* Description: Monitor/resolution data from Steam used for
*              testing.
*
*****************************************************************/
#pragma once

// gfx
#include "src/gfx/cartesian.hpp"

// C++ standard library
#include <vector>

namespace testing {

std::vector<gfx::size> const& steam_resolutions();

} // namespace testing
