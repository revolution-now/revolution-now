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

// gfx
#include "resolution-enum.hpp"

// C++ standard library
#include <unordered_set>

using namespace std;

namespace gfx {

namespace {

using ::base::maybe;
using ::base::nothing;

struct LogicalResolution {
  size dimensions = {};
  int scale       = {};
};

LogicalResolution construct_logical( size const dimensions,
                                     int const scale ) {
  return LogicalResolution{ .dimensions = dimensions,
                            .scale      = scale };
}

LogicalResolution construct_logical(
    e_resolution const resolution, int const scale ) {
  size const dimensions = resolution_size( resolution );
  return construct_logical( dimensions, scale );
}

bool is_exact( Resolution const& resolution ) {
  return resolution.viewport.origin.distance_from_origin() ==
         size{};
}

vector<LogicalResolution> logical_resolutions_for_physical(
    size const physical_window ) {
  vector<LogicalResolution> resolutions;
  auto const min_dimension =
      std::min( physical_window.w, physical_window.h );
  for( int i = 1; i <= min_dimension; ++i )
    if( physical_window.w % i == 0 &&
        physical_window.h % i == 0 )
      resolutions.push_back(
          construct_logical( physical_window / i, i ) );
  return resolutions;
}

maybe<double> pixel_size_millimeters(
    maybe<MonitorDpi const&> const dpi, int const scale ) {
  CHECK_GT( scale, 0 );
  if( !dpi.has_value() ) return nothing;
  double constexpr kMillimetersPerInch = 25.4;
  // We are assuming that horizontal == vertical here.
  return scale * kMillimetersPerInch / dpi->horizontal;
}

// This function has the following desirable properties:
//
//   1. It is symetric in x and y.
//   2. If either one is zero, the result is 0.
//   3. If either one is +/- inf, the result is 0.
//   4. If they are equal, the result is 1.
//   5. The value is in [0, 1].
//
// This is used to generate a "score" that measure the difference
// between the two in a way that maps onto [0, 1] and where 0 is
// "worst match" and 1 is "best match".
double relative_diff_score_fn( double const x, double const y ) {
  // The clamp is just for floating point error; the equation
  // theoretically guarantees that the result will be in [0,1.0]
  // and so this would at most eliminate insignificant fractions.
  return std::clamp( 1.0 - abs( x - y ) / ( x + y ), 0.0, 1.0 );
}

ResolutionScores score(
    Resolution const& r,
    ResolutionScoringOptions const& options ) {
  ResolutionScores scores;

  if( r.physical_window.area() == 0 ) return scores;

  auto round_score = []( double& score ) {
    score = double( lround( score * 100.0 ) ) / 100.0;
  };

  scores = {};

  // Fitting score.
  int const occupied_area = r.viewport.area();
  int const total_area    = r.physical_window.area();
  CHECK_LE( occupied_area, total_area );
  scores.fitting = relative_diff_score_fn( sqrt( occupied_area ),
                                           sqrt( total_area ) );

  // Pixel size score.
  if( r.pixel_size.has_value() ) {
    CHECK_GE( *r.pixel_size, 0.0 );
    scores.pixel_size = relative_diff_score_fn(
        *r.pixel_size, options.ideal_pixel_size_mm );
  }

  // Overall score should be computed last.
  auto const fields = {
    &scores.fitting,    //
    &scores.pixel_size, //
    // Leave out the overall score.
  };

  // Round them all to two decimal places to allow some amount of
  // bucketing, that way we can have a chance of being able to
  // say "if these two have the same fitting score, then defer to
  // pixel size score".
  for( auto const field : fields ) round_score( *field );

  scores.overall = 1.0;
  for( auto const field : fields ) scores.overall *= ( *field );
  round_score( scores.overall );

  return scores;
}

bool meets_tolerance( ScoredResolution const& scored_resolution,
                      ResolutionScoringOptions const& options ) {
  auto const& r         = scored_resolution.resolution;
  auto const& scores    = scored_resolution.scores;
  auto const& tolerance = options.tolerance;
  if( tolerance.min_percent_covered.has_value() &&
      r.physical_window.area() > 0 ) {
    int const scaled_clipped_area =
        r.logical.area() * ( r.scale * r.scale );
    double const percent_covered =
        scaled_clipped_area /
        static_cast<double>( r.physical_window.area() );
    if( percent_covered < *tolerance.min_percent_covered )
      return false;
  }

  if( tolerance.fitting_score_cutoff.has_value() ) {
    if( scores.fitting < *tolerance.fitting_score_cutoff )
      return false;
  }
  return true;
}

vector<Resolution> find_resolutions(
    Monitor const& monitor, size const physical_window ) {
  rect const physical_rect{ .origin = {},
                            .size   = physical_window };

  vector<LogicalResolution> const choices =
      logical_resolutions_for_physical( physical_window );
  vector<Resolution> res;
  for( e_resolution const target : supported_resolutions() ) {
    LogicalResolution scaled = construct_logical( target, 1 );
    while( scaled.dimensions.fits_inside( physical_window ) ) {
      LogicalResolution const logical =
          construct_logical( target, scaled.scale );
      // Exact fit to physical window.
      bool const exact_fit =
          ( logical.dimensions * logical.scale ==
            physical_window );
      Resolution r;
      r.physical_window = physical_window;
      r.logical         = logical.dimensions;
      r.scale           = logical.scale;
      r.viewport        = physical_rect;
      r.named           = target;
      r.pixel_size =
          pixel_size_millimeters( monitor.dpi, logical.scale );

      if( !exact_fit ) {
        // Fits within the window but smaller. Need to adjust
        // viewport.
        size const chosen_physical =
            logical.dimensions * logical.scale;
        rect const viewport{
          .origin =
              centered_in( chosen_physical, physical_rect ),
          .size = chosen_physical };
        r.viewport = viewport;
      }

      res.push_back( std::move( r ) );
      ++scaled.scale;
      scaled.dimensions =
          resolution_size( target ) * scaled.scale;
    }
  }
  return res;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
vector<ScoredResolution> resolution_analysis(
    ResolutionAnalysisOptions const& options ) {
  vector<ScoredResolution> all;
  auto const resolutions = find_resolutions(
      options.monitor, options.physical_window );
  for( auto const& r : resolutions )
    all.push_back( ScoredResolution{
      .resolution = r,
      .scores     = score( r, options.scoring_options ) } );

  auto stable_sort_by = [&]( auto&& key_fn ) {
    ranges::stable_sort( all,
                         [&]( auto const& l, auto const& r ) {
                           return key_fn( l, r );
                         } );
  };

  // Sort in reverse order of importance. The less important once
  // are not really ever expected to be relevant, since they'd
  // only be consulted if the overall scores for two candidates
  // turn out to be the same, which seems kind of rare.

  stable_sort_by( []( ScoredResolution const& l,
                      ScoredResolution const& r ) {
    return l.scores.pixel_size > r.scores.pixel_size;
  } );

  stable_sort_by( []( ScoredResolution const& l,
                      ScoredResolution const& r ) {
    return l.scores.fitting > r.scores.fitting;
  } );

  stable_sort_by( []( ScoredResolution const& l,
                      ScoredResolution const& r ) {
    return l.scores.overall > r.scores.overall;
  } );

  if( options.scoring_options.prefer_fullscreen ) {
    stable_sort_by( []( ScoredResolution const& l,
                        ScoredResolution const& r ) {
      if( is_exact( l.resolution ) == is_exact( r.resolution ) )
        return false;
      return is_exact( l.resolution );
    } );
  }

  vector<ScoredResolution> res;
  unordered_set<size> seen;
  for( ScoredResolution const& rr : all ) {
    size const dimensions = rr.resolution.logical;
    bool const skip = options.scoring_options.remove_redundant &&
                      seen.contains( dimensions );
    if( meets_tolerance( rr, options.scoring_options ) &&
        !skip ) {
      seen.insert( dimensions );
      res.push_back( rr );
    }
  }
  return res;
}

} // namespace gfx
