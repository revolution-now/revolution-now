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

// config
#include "config/resolutions.rds.hpp"

// refl
#include "refl/enum-map.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::maybe;

/****************************************************************
** Globals.
*****************************************************************/
constexpr gfx::size resolution_size( e_resolution const r ) {
  switch( r ) {
    case e_resolution::_640x360:
      return { .w = 640, .h = 360 };
    case e_resolution::_640x400:
      return { .w = 640, .h = 400 };
    case e_resolution::_768x432:
      return { .w = 768, .h = 432 };
  }
}

refl::enum_map<e_resolution, gfx::size> const
    kResolutionSizeMap = [] {
      refl::enum_map<e_resolution, gfx::size> res;
      for( auto const r : refl::enum_values<e_resolution> )
        res[r] = resolution_size( r );
      return res;
    }();

unordered_map<gfx::size, e_resolution> const
    kResolutionReverseSizeMap = [] {
      unordered_map<gfx::size, e_resolution> res;
      for( auto const& [k, v] : kResolutionSizeMap ) res[v] = k;
      return res;
    }();

vector<gfx::size> const kResolutionSizes = [] {
  vector<gfx::size> res;
  for( auto const& [r, sz] : kResolutionSizeMap )
    res.push_back( sz );
  return res;
}();

gfx::ResolutionRatingOptions const RESOLUTION_RATINGS{
  // TODO: consider making this a config parameter.
  .prefer_fullscreen = true,
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
  gfx::ResolutionAnalysisOptions const options{
    .monitor                      = monitor,
    .physical_window              = physical_window,
    .supported_logical_dimensions = kResolutionSizes,
    .rating_options               = RESOLUTION_RATINGS };
  gfx::ResolutionRatings ratings =
      resolution_analysis( options );
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
    auto const& logical_size =
        ratings.available[0].resolution.logical.dimensions;
    auto const named_it =
        kResolutionReverseSizeMap.find( logical_size );
    CHECK( named_it != kResolutionReverseSizeMap.end() );
    // Copy to avoid use-after-move.
    auto selected = SelectedResolution{
      .rated        = ratings.available[0],
      .idx          = 0,
      .availability = e_resolution_availability::available,
      .named        = named_it->second };
    return Resolutions{ .ratings  = std::move( ratings ),
                        .selected = std::move( selected ) };
  } else {
    // Copy to avoid use-after-move.
    auto selected = SelectedResolution{
      .rated        = ratings.unavailable[0],
      .idx          = 0,
      .availability = e_resolution_availability::unavailable,
      .named        = nothing };
    return Resolutions{ .ratings  = std::move( ratings ),
                        .selected = std::move( selected ) };
  }
}

} // namespace rn
