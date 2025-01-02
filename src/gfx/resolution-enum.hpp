/****************************************************************
**resolution-enum.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-02.
*
* Description: Enumerates resolutions that may be supported.
*
*****************************************************************/
#pragma once

// rds
#include "resolution-enum.rds.hpp"

// gfx
#include "cartesian.hpp"

namespace gfx {

/****************************************************************
** e_resolution
*****************************************************************/
size resolution_size( e_resolution resolution );

std::vector<e_resolution> const& supported_resolutions();

base::maybe<e_resolution> resolution_from_size( size sz );

} // namespace gfx
