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
#include "gfx/logical.hpp"
#include "logical.rds.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::maybe;

/****************************************************************
** Globals.
*****************************************************************/
vector<gfx::size> const SUPPORTED_LOGICAL_RESOLUTIONS{
  // 16:9
  { .w = 768, .h = 432 },
  { .w = 640, .h = 360 },
  // { .w = 480, .h = 270 }, // superlarge.

  // 16:10
  { .w = 720, .h = 450 },
  { .w = 576, .h = 360 },
  { .w = 640, .h = 400 },

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

gfx::ResolutionRatingOptions const RESOLUTION_RATINGS{
  // TODO: consider making this a config parameter.
  .prefer_fullscreen = false,
  .tolerance         = { .min_percent_covered  = nothing,
                         .fitting_score_cutoff = nothing },
  // TODO: see if there is a better way to come up with this
  // number instead of hardcoding it for all monitor sizes.
  // .ideal_pixel_size_mm = .79375, // selects 640x360
  .ideal_pixel_size_mm = .66145, // selects 768x432
  // Just take the best of each logical resolution instead of in-
  // cluding all possible scales of it that fit.
  .remove_redundant = true,
};

/****************************************************************
** Helpers.
*****************************************************************/
gfx::ResolutionRatings compute_logical_resolution_ratings(
    gfx::Monitor const& monitor,
    gfx::size const     physical_window ) {
  gfx::ResolutionAnalysis const analysis = resolution_analysis(
      monitor, physical_window, SUPPORTED_LOGICAL_RESOLUTIONS );
  gfx::ResolutionRatings ratings = resolution_ratings(
      analysis, resolution_rating_options() );
  // Always make sure that we have at least one unavailable reso-
  // lution since it is the last resort. Just choose the empty
  // resolution.
  ratings.unavailable.push_back( gfx::RatedResolution{
    .resolution = gfx::Resolution{ .physical_window = {},
                                   .logical = { .dimensions = {},
                                                .scale = 1 },
                                   .viewport = {} },
    .scores     = {} } );
  return ratings;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
gfx::ResolutionRatingOptions resolution_rating_options() {
  return RESOLUTION_RATINGS;
}

Resolutions compute_resolutions(
    gfx::Monitor const& monitor,
    gfx::size const     physical_window ) {
  auto ratings = compute_logical_resolution_ratings(
      monitor, physical_window );
  // This should be guaranteed by the method that produces the
  // ratings, and ensure that we always have at least one fall-
  // back.
  CHECK( !ratings.unavailable.empty() );
  if( !ratings.available.empty() ) {
    // Copy to avoid use-after-move.
    auto selected = SelectedResolution{
      .rated        = ratings.available[0],
      .idx          = 0,
      .availability = e_resolution_availability::available };
    return Resolutions{ .ratings  = std::move( ratings ),
                        .selected = std::move( selected ) };
  } else {
    // Copy to avoid use-after-move.
    auto selected = SelectedResolution{
      .rated        = ratings.unavailable[0],
      .idx          = 0,
      .availability = e_resolution_availability::unavailable };
    return Resolutions{ .ratings  = std::move( ratings ),
                        .selected = std::move( selected ) };
  }
}

} // namespace rn
