/****************************************************************
**resolution.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-14.
*
* Description: Handles things related to the physical and logical
*              resolutions used by the game.
*
*****************************************************************/
#include "resolution.hpp"

// gfx
#include "gfx/aspect.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::maybe;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
gfx::ResolutionRatings compute_logical_resolution_ratings(
    gfx::size physical_size ) {
  vector<gfx::Resolution> res;
  vector<gfx::size> const target_logical_resolutions{
    // 16:9
    { .w = 768, .h = 432 },
    { .w = 640, .h = 360 },
    // { .w = 480, .h = 270 }, // superlarge.

    // 9:16 (rotated 16:9)
    { .w = 432, .h = 768 },
    { .w = 360, .h = 640 },
    // { .w = 270, .h = 480 }, // superlarge.

    // 16:10
    { .w = 720, .h = 450 },
    { .w = 576, .h = 360 },
    { .w = 640, .h = 400 },

    // 10:16 (rotated 16:10)
    { .w = 450, .h = 720 },
    { .w = 360, .h = 576 },
    { .w = 400, .h = 640 },

    // 4:3
    { .w = 960, .h = 720 },
    { .w = 640, .h = 480 },

    // 21:9 (approximately)
    //
    // - for native: 2560x1080 (WFHD)
    { .w = 1'280, .h = 540 },
    { .w = 852, .h = 360 }, // not exact; w=853.333333
    //
    // - for native: 3440x1440 (WQHD)
    { .w = 1'146, .h = 480 }, // not exact; w=1146.67
    { .w = 860, .h = 360 },
    //
    // - for native: 3840x1600
    { .w = 960, .h = 400 },
  };

  // Common ultrawide resolutions.

  gfx::ResolutionAnalysis const analysis = resolution_analysis(
      target_logical_resolutions, physical_size );

  gfx::ResolutionTolerance const tolerance{
    .min_percent_covered  = nothing, // .8,
    .fitting_score_cutoff = nothing,
  };

  return resolution_ratings( analysis, tolerance );
}

maybe<gfx::Resolution> recompute_best_logical_resolution(
    gfx::size const physical_size ) {
  auto ratings =
      compute_logical_resolution_ratings( physical_size );
  if( ratings.available.empty() ) return nothing;
  return std::move( ratings.available[0] );
}

maybe<gfx::Resolution>
recompute_best_unavailable_logical_resolution(
    gfx::size physical_size ) {
  auto ratings =
      compute_logical_resolution_ratings( physical_size );
  if( ratings.unavailable.empty() ) return nothing;
  return std::move( ratings.unavailable[0] );
}

} // namespace rn
