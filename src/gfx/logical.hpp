/****************************************************************
**logical.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-25.
*
* Description: Logic for dealing with logical resolutions.
*
*****************************************************************/
#pragma once

// rds
#include "logical.rds.hpp"

// C++ standard library
#include <span>
#include <string>

namespace gfx {

/****************************************************************
** Public API.
*****************************************************************/
ResolutionScores score( Resolution const resolution );

ResolutionAnalysis resolution_analysis(
    std::span<size const> target_logical_resolutions,
    size                  physical_resolution );

ResolutionRatings resolution_ratings(
    ResolutionAnalysis const&  analysis,
    ResolutionTolerance const& tolerance );

} // namespace gfx
