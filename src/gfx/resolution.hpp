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

// base
#include "base/maybe.hpp"

namespace gfx {

/****************************************************************
** e_resolution
*****************************************************************/
size resolution_size( e_resolution resolution );

std::vector<size> const& supported_resolutions();

base::maybe<e_resolution> resolution_from_size( size sz );

/****************************************************************
** Resolution Selection.
*****************************************************************/
Resolutions compute_resolutions( Monitor const& monitor,
                                 size physical_window );

SelectedResolution create_selected_available_resolution(
    ResolutionRatings const& ratings, int idx );

} // namespace gfx
