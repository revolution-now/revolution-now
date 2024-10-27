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
using ::base::nothing;

static bool meets_tolerance(
    Resolution const&              r,
    ResolutionRatingOptions const& options ) {
  auto const& tolerance = options.tolerance;
  if( tolerance.min_percent_covered.has_value() &&
      r.physical_window.area() > 0 ) {
    int const scaled_clipped_area =
        r.logical.dimensions.area() *
        ( r.logical.scale * r.logical.scale );
    double const percent_covered =
        scaled_clipped_area /
        static_cast<double>( r.physical_window.area() );
    if( percent_covered < *tolerance.min_percent_covered )
      return false;
  }

  if( tolerance.fitting_score_cutoff.has_value() ) {
    auto const scores = score( r, options );
    if( scores.fitting < *tolerance.fitting_score_cutoff )
      return false;
  }
  return true;
}

bool is_exact( Resolution const& resolution ) {
  return resolution.viewport.origin.distance_from_origin() ==
         size{};
}

vector<LogicalResolution> logical_resolutions_for_physical(
    size const physical_window ) {
  vector<LogicalResolution> resolutions;
  auto const                min_dimension =
      std::min( physical_window.w, physical_window.h );
  for( int i = 1; i <= min_dimension; ++i )
    if( physical_window.w % i == 0 &&
        physical_window.h % i == 0 )
      resolutions.push_back( LogicalResolution{
        .dimensions = physical_window / i, .scale = i } );
  return resolutions;
}

maybe<double> pixel_size_millimeters( maybe<double> const dpi,
                                      int const scale ) {
  CHECK_GT( scale, 0 );
  if( !dpi.has_value() ) return nothing;
  double constexpr kMillimetersPerInch = 25.4;
  return scale * kMillimetersPerInch / *dpi;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
Monitor monitor_properties( size const          physical_screen,
                            maybe<double> const dpi ) {
  Monitor monitor;
  monitor.physical_screen = physical_screen;
  if( dpi.has_value() ) {
    monitor.dpi = dpi;
    monitor.diagonal_inches =
        sqrt( pow( physical_screen.w, 2.0 ) +
              pow( physical_screen.h, 2.0 ) ) /
        *dpi;
  }
  return monitor;
}

ResolutionScores score(
    Resolution const&              r,
    ResolutionRatingOptions const& options ) {
  ResolutionScores scores;

  if( r.physical_window.area() == 0 ) return {};

  scores = {};

  // Fitting score.
  int const occupied_area = r.viewport.area();
  int const total_area    = r.physical_window.area();
  CHECK_LE( occupied_area, total_area );
  scores.fitting = sqrt( 1.0 * occupied_area / total_area );

  // Size score.
  if( r.pixel_size_mm.has_value() ) {
    CHECK_GE( *r.pixel_size_mm, 0.0 );
    double const ideal = options.ideal_pixel_size_mm;
    // This has the property that is the pixel size is zero or
    // infinity then the score is zero, and the score is 1.0 when
    // the pixel size matches the ideal value.
    scores.pixel_size = 1.0 - abs( *r.pixel_size_mm - ideal ) /
                                  ( *r.pixel_size_mm + ideal );
    CHECK_GE( scores.pixel_size, 0.0 );
  }

  // Overall score should be computed last.
  scores.overall = scores.fitting * scores.pixel_size;
  return scores;
}

ResolutionAnalysis resolution_analysis(
    Monitor const& monitor, size const physical_window,
    std::span<size const> supported_logical_resolutions ) {
  rect const physical_rect{ .origin = {},
                            .size   = physical_window };

  vector<LogicalResolution> const choices =
      logical_resolutions_for_physical( physical_window );
  ResolutionAnalysis res;
  for( size const target : supported_logical_resolutions ) {
    LogicalResolution scaled{ .dimensions = target, .scale = 1 };
    maybe<LogicalResolution> largest_that_fits;
    while( scaled.dimensions.fits_inside( physical_window ) ) {
      largest_that_fits = LogicalResolution{
        .dimensions = target, .scale = scaled.scale };
      ++scaled.scale;
      scaled.dimensions = target * scaled.scale;
    }
    if( !largest_that_fits.has_value() ) continue;
    LogicalResolution const& logical = *largest_that_fits;

    if( logical.dimensions * logical.scale == physical_window ) {
      // Exact fit to physical_window window.
      res.resolutions.push_back(
          Resolution{ .physical_window = physical_window,
                      .logical         = logical,
                      .viewport        = physical_rect,
                      .pixel_size_mm   = pixel_size_millimeters(
                          monitor.dpi, logical.scale ) } );
    } else {
      // Fits within the window but smaller.
      size const chosen_physical =
          logical.dimensions * logical.scale;
      rect const viewport{
        .origin = centered_in( chosen_physical, physical_rect ),
        .size   = chosen_physical };
      res.resolutions.push_back(
          Resolution{ .physical_window = physical_window,
                      .logical         = logical,
                      .viewport        = viewport,
                      .pixel_size_mm   = pixel_size_millimeters(
                          monitor.dpi, logical.scale ) } );
    }
  }
  return res;
}

ResolutionRatings resolution_ratings(
    ResolutionAnalysis const&      analysis,
    ResolutionRatingOptions const& options ) {
  ResolutionRatings res;

  auto sorter = [&]( Resolution const& l, Resolution const& r ) {
    if( options.prefer_fullscreen &&
        is_exact( l ) != is_exact( r ) )
      // Exact fits should go first.
      return is_exact( l );
    return score( l, options ).overall >
           score( r, options ).overall;
  };

  auto const sorted = [&] {
    auto cpy = analysis.resolutions;
    ranges::sort( cpy, sorter );
    return cpy;
  }();

  for( Resolution const& r : sorted ) {
    auto& where = meets_tolerance( r, options )
                      ? res.available
                      : res.unavailable;
    where.push_back( r );
  }

  return res;
}

} // namespace gfx
