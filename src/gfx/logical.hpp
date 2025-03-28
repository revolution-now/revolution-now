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

// gfx
#include "gfx/monitor.rds.hpp"

namespace gfx {

/****************************************************************
** Resolution Selection Parameters.
*****************************************************************/
struct ResolutionTolerance {
  base::maybe<double> min_percent_covered  = {};
  base::maybe<double> fitting_score_cutoff = {};
};

struct ResolutionScoringOptions {
  bool prefer_fullscreen        = {};
  ResolutionTolerance tolerance = {};
  double ideal_pixel_size_mm    = {};
  // When this is true, only one of each logical resolution will
  // be retained, namely the one with the best score. In practice
  // this means that, for a given logical resolution, only the
  // one with the largest scale factor will be kept. That said,
  // it technically depends on the pixel size score as well.
  bool remove_redundant = {};
};

struct ResolutionAnalysisOptions {
  Monitor monitor                          = {};
  size physical_window                     = {};
  ResolutionScoringOptions scoring_options = {};
};

/****************************************************************
** Public API.
*****************************************************************/
std::vector<ScoredResolution> resolution_analysis(
    ResolutionAnalysisOptions const& options );

} // namespace gfx
