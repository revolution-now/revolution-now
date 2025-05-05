/****************************************************************
**resolution.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Manages the set of fixed resolutions.
*
*****************************************************************/
#include "resolution.hpp"

// gfx
#include "logical.hpp"
#include "resolution-enum.hpp"

// refl
#include "refl/query-enum.hpp"

using namespace std;

namespace gfx {

namespace {

using ::base::maybe;
using ::base::nothing;

/****************************************************************
** Globals.
*****************************************************************/
vector<e_resolution> const kResolutionSizes = [] {
  vector<e_resolution> res;
  for( auto const r : refl::enum_values<e_resolution> )
    res.push_back( r );
  return res;
}();

ResolutionScoringOptions const RESOLUTION_RATINGS{
  // TODO: consider making this a config parameter.
  .prefer_fullscreen = true,
  .tolerance         = { .min_percent_covered  = nothing,
                         .fitting_score_cutoff = nothing },
  // TODO: see if there is a better way to come up with this
  // number instead of hardcoding it for all monitor sizes.
  // .ideal_pixel_size_mm = .66145, // selects 768x432
  .ideal_pixel_size_mm = .79375, // selects 640x360
  // Just take the best of each logical resolution instead of in-
  // cluding all possible scales of it that fit.
  .remove_redundant = true,
};

/****************************************************************
** Helpers.
*****************************************************************/
vector<ScoredResolution> compute_logical_resolution_ratings(
    Monitor const& monitor, size const physical_window ) {
  ResolutionAnalysisOptions const options{
    .monitor         = monitor,
    .physical_window = physical_window,
    .scoring_options = RESOLUTION_RATINGS };
  return resolution_analysis( options );
}

} // namespace

/****************************************************************
** Resolution Selection.
*****************************************************************/
Resolutions compute_resolutions( Monitor const& monitor,
                                 size const physical_window ) {
  Resolutions res;
  res.scored = compute_logical_resolution_ratings(
      monitor, physical_window );
  if( !res.scored.empty() ) res.selected = 0;
  return res;
}

} // namespace gfx
