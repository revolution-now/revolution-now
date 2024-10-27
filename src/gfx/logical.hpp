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
Monitor monitor_properties( size                physical_screen,
                            base::maybe<double> dpi );

ResolutionScores score( Resolution const& resolution,
                        Monitor const&    monitor );

ResolutionAnalysis resolution_analysis(
    size                  physical_window,
    std::span<size const> supported_logical_resolutions );

ResolutionRatings resolution_ratings(
    ResolutionAnalysis const& analysis, Monitor const& monitor,
    ResolutionTolerance const& tolerance );

} // namespace gfx
