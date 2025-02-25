/****************************************************************
**resolution.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Manages the set of fixed resolutions.
*
*****************************************************************/
#pragma once

// rds
#include "resolution.rds.hpp"

// gfx
#include "cartesian.hpp"
#include "monitor.rds.hpp"

// base
#include "base/maybe.hpp"

namespace gfx {

/****************************************************************
** Resolution Selection.
*****************************************************************/
Resolutions compute_resolutions( Monitor const& monitor,
                                 size physical_window );

} // namespace gfx
