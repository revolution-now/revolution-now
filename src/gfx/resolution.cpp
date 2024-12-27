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

// refl
#include "refl/query-enum.hpp"

// base
#include "base/keyval.hpp"

using namespace std;

namespace gfx {

namespace {

using ::base::maybe;
using ::base::nothing;

unordered_map<size, e_resolution> const
    kResolutionReverseSizeMap = [] {
      unordered_map<size, e_resolution> res;
      for( auto const r : refl::enum_values<e_resolution> )
        res[resolution_size( r )] = r;
      return res;
    }();

/****************************************************************
** Globals.
*****************************************************************/
vector<size> const kResolutionSizes = [] {
  vector<size> res;
  for( auto const r : refl::enum_values<e_resolution> )
    res.push_back( resolution_size( r ) );
  return res;
}();

ResolutionRatingOptions const RESOLUTION_RATINGS{
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
ResolutionRatings compute_logical_resolution_ratings(
    Monitor const& monitor, size const physical_window ) {
  ResolutionAnalysisOptions const options{
    .monitor                      = monitor,
    .physical_window              = physical_window,
    .rating_options               = RESOLUTION_RATINGS,
    .supported_logical_dimensions = kResolutionSizes };
  ResolutionRatings ratings = resolution_analysis( options );
  // Always make sure that we have at least one unavailable reso-
  // lution since it is the last resort. Just choose the empty
  // resolution.
  ratings.unavailable.push_back( ScoredResolution{
    .resolution = Resolution{ .physical_window = {},
                              .logical         = {},
                              .scale           = 1,
                              .viewport        = {} },
    .scores     = {} } );
  return ratings;
}

} // namespace

/****************************************************************
** e_resolution
*****************************************************************/
size resolution_size( e_resolution const r ) {
  switch( r ) {
    case e_resolution::_640x360:
      return { .w = 640, .h = 360 };
    case e_resolution::_640x400:
      return { .w = 640, .h = 400 };
    case e_resolution::_768x432:
      return { .w = 768, .h = 432 };
    case e_resolution::_576x360:
      return { .w = 576, .h = 360 };
    case e_resolution::_720x450:
      return { .w = 720, .h = 450 };
  }
}

vector<size> const& supported_resolutions() {
  static vector<size> const v = [] {
    vector<size> res;
    res.reserve( refl::enum_count<e_resolution> );
    for( e_resolution const r : refl::enum_values<e_resolution> )
      res.push_back( resolution_size( r ) );
    return res;
  }();
  return v;
}

maybe<e_resolution> resolution_from_size( size const sz ) {
  return base::lookup( kResolutionReverseSizeMap, sz );
}

Resolutions compute_resolutions( Monitor const& monitor,
                                 size physical_window );

SelectedResolution create_selected_available_resolution(
    ResolutionRatings const& ratings, int idx );

/****************************************************************
** Resolution Selection.
*****************************************************************/
Resolutions compute_resolutions( Monitor const& monitor,
                                 size const physical_window ) {
  auto ratings = compute_logical_resolution_ratings(
      monitor, physical_window );
  // This should be guaranteed by the method that produces the
  // ratings, and ensure that we always have at least one fall-
  // back.
  CHECK( !ratings.unavailable.empty() );
  if( !ratings.available.empty() ) {
    // Copy to avoid use-after-move.
    auto selected =
        create_selected_available_resolution( ratings, 0 );
    return Resolutions{ .ratings  = std::move( ratings ),
                        .selected = std::move( selected ) };
  } else {
    // Copy to avoid use-after-move.
    auto selected =
        SelectedResolution{ .rated     = ratings.unavailable[0],
                            .idx       = 0,
                            .available = false,
                            .named     = nothing };
    return Resolutions{ .ratings  = std::move( ratings ),
                        .selected = std::move( selected ) };
  }
}

SelectedResolution create_selected_available_resolution(
    ResolutionRatings const& ratings, int const idx ) {
  SelectedResolution selected;
  auto const& available = ratings.available;
  CHECK_LT( idx, ssize( available ) );
  selected.rated     = available[idx];
  selected.idx       = idx;
  selected.available = true;
  UNWRAP_CHECK_T(
      selected.named,
      resolution_from_size(
          available[selected.idx].resolution.logical ) );
  return selected;
}

} // namespace gfx
