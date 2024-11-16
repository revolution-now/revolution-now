/****************************************************************
**resolutions.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-11-02.
*
* Description: Supported logical resolutions.
*
*****************************************************************/
#pragma once

// rds
#include "resolutions.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"

// C++ standard library
#include <vector>

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
gfx::size resolution_size( e_resolution resolution );

std::vector<gfx::size> const& supported_resolutions();

base::maybe<e_resolution> resolution_from_size( gfx::size sz );

} // namespace rn
