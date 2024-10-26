/****************************************************************
**logical.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-25.
*
* Description: Logic for dealing with logical resolutions.
*
*****************************************************************/
#include "logical.hpp"

using namespace std;

namespace gfx {

namespace {

using ::base::maybe;

static bool meets_tolerance(
    ScoredResolution const&    scored_resolution,
    ResolutionTolerance const& tolerance ) {
  auto const& r      = scored_resolution.resolution;
  auto const& scores = scored_resolution.scores;
  if( tolerance.min_percent_covered.has_value() &&
      r.physical.area() > 0 ) {
    int const scaled_clipped_area =
        r.logical.dimensions.area() *
        ( r.logical.scale * r.logical.scale );
    double const percent_covered =
        scaled_clipped_area /
        static_cast<double>( r.physical.area() );
    if( percent_covered < *tolerance.min_percent_covered )
      return false;
  }

  if( tolerance.fitting_score_cutoff.has_value() ) {
    if( scores.fitting > *tolerance.fitting_score_cutoff )
      return false;
  }
  return true;
}

void add_scores( ScoredResolution& scored_resolution ) {
  scored_resolution.scores =
      score( scored_resolution.resolution );
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
vector<LogicalResolution> supported_logical_resolutions(
    size const max_resolution ) {
  vector<LogicalResolution> resolutions;
  auto const                min_dimension =
      std::min( max_resolution.w, max_resolution.h );
  for( int i = 1; i <= min_dimension; ++i )
    if( max_resolution.w % i == 0 && max_resolution.h % i == 0 )
      resolutions.push_back( LogicalResolution{
        .dimensions = max_resolution / i, .scale = i } );
  return resolutions;
}

bool is_exact( Resolution const& resolution ) {
  return resolution.viewport.origin.distance_from_origin() ==
         size{};
}

ResolutionScores score( Resolution const resolution ) {
  ResolutionScores scores;
  auto const&      r = resolution;

  if( resolution.physical.area() == 0 ) return {};

  scores = {};

  // Fitting score.
  if( !is_exact( r ) )
    scores.fitting =
        ( r.logical.dimensions * r.logical.scale - r.physical )
            .pythagorean() /
        r.physical.pythagorean();

  // Size score.
  // TODO

  // Overall score should be computed last.
  // TODO: combine above scores to produce overall score.
  scores.overall = scores.fitting;
  return scores;
}

ResolutionAnalysis resolution_analysis(
    span<size const> const target_logical_resolutions,
    size const             physical ) {
  rect const physical_rect{ .origin = {}, .size = physical };
  vector<LogicalResolution> const choices =
      supported_logical_resolutions( physical );
  ResolutionAnalysis res;
  for( size const target : target_logical_resolutions ) {
    LogicalResolution scaled{ .dimensions = target, .scale = 1 };
    maybe<LogicalResolution> largest_that_fits;
    while( scaled.dimensions.fits_inside( physical ) ) {
      largest_that_fits = LogicalResolution{
        .dimensions = target, .scale = scaled.scale };
      ++scaled.scale;
      scaled.dimensions = target * scaled.scale;
    }
    if( !largest_that_fits.has_value() ) continue;
    LogicalResolution const& logical = *largest_that_fits;

    if( logical.dimensions * logical.scale == physical ) {
      // Exact fit to physical window.
      res.resolutions.push_back( ScoredResolution{
        .resolution = { .physical = physical,
                        .logical  = logical,
                        .viewport = physical_rect } } );
    } else {
      // Fits within the window but smaller.
      size const chosen_physical =
          logical.dimensions * logical.scale;
      rect const viewport{
        .origin = centered_in( chosen_physical, physical_rect ),
        .size   = chosen_physical };
      res.resolutions.push_back( ScoredResolution{
        .resolution = { .physical = physical,
                        .logical  = logical,
                        .viewport = viewport } } );
    }
    add_scores( res.resolutions.back() );
  }
  return res;
}

ResolutionRatings resolution_ratings(
    ResolutionAnalysis const&  analysis,
    ResolutionTolerance const& tolerance ) {
  ResolutionRatings res;

  auto sorted = analysis.resolutions;
  sort( sorted.begin(), sorted.end(),
        []( ScoredResolution const& l,
            ScoredResolution const& r ) {
          if( is_exact( l.resolution ) !=
              is_exact( r.resolution ) )
            // Exact fits should go first.
            return is_exact( l.resolution );
          if( is_exact( l.resolution ) ) {
            // FIXME: this needs to be improved to account for
            // the angular size of pixels. Such angular size
            // might also need to be weighed in to the score for
            // inexact fits. These need to be given a `score`
            // field like the inexact fits.
            return l.resolution.logical.dimensions.area() >
                   r.resolution.logical.dimensions.area();
          } else {
            return l.scores.overall < r.scores.overall;
          }
        } );

  for( auto const& scored_resolution : sorted ) {
    auto& where = meets_tolerance( scored_resolution, tolerance )
                      ? res.available
                      : res.unavailable;
    where.push_back( scored_resolution.resolution );
  }

  return res;
}

} // namespace gfx
