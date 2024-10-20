/****************************************************************
**aspect.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-09.
*
* Description: Handles screen aspect ratio and resolution selec-
*              tion.
*
*****************************************************************/
#include "aspect.hpp"

// refl
#include "aspect.rds.hpp"
#include "refl/query-enum.hpp"

// C++ standard library
#include <numeric>

using namespace std;

namespace gfx {

namespace {

using ::base::maybe;
using ::base::nothing;

maybe<double> is_close( double const l, double const r,
                        double const tolerance ) {
  // Since the ratio involves two numbers, then in the worst case
  // they could both be off by `tolerance` but in different di-
  // rections, which means that the tolerance for the ratio is
  // 2*tolerance.
  double const ratio_tolerance = tolerance * 2;
  double const ratio           = l / r;
  if( ratio >= 1.0 / ( 1.0 + ratio_tolerance ) &&
      ratio <= 1.0 + ratio_tolerance )
    return abs( ratio - 1.0 );
  return nothing;
}

maybe<double> is_close( AspectRatio const l, AspectRatio const r,
                        double const tolerance ) {
  return is_close( l.scalar(), r.scalar(), tolerance );
}

static bool meets_tolerance(
    Resolution const& r, ResolutionTolerance const& tolerance ) {
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
    if( r.scores.fitting > *tolerance.fitting_score_cutoff )
      return false;
  }
  return true;
}

void compute_scores( Resolution& r ) {
  if( r.physical.area() == 0 ) return;
  auto& scores = r.scores;

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
  r.scores.overall = r.scores.fitting;
}

} // namespace

/****************************************************************
** AspectRatio
*****************************************************************/
maybe<AspectRatio> AspectRatio::from_size(
    size const resolution ) {
  if( resolution.w <= 0 ) return nothing;
  if( resolution.h <= 0 ) return nothing;
  auto const common = gcd( resolution.w, resolution.h );
  size const reduced{ .w = resolution.w / common,
                      .h = resolution.h / common };
  return AspectRatio( reduced );
}

AspectRatio::AspectRatio( size resolution )
  : ratio_( resolution ) {
  CHECK_GT( resolution.h, 0 );
  CHECK_GT( resolution.w, 0 );
}

double AspectRatio::scalar() const {
  CHECK_GT( ratio_.h, 0 );
  return ratio_.w / double( ratio_.h );
}

AspectRatio AspectRatio::from_named(
    e_named_aspect_ratio const ratio ) {
  switch( ratio ) {
    case e_named_aspect_ratio::w_h_16_10: {
      static AspectRatio const r( size{ .w = 8, .h = 5 } );
      return r;
    }
    case e_named_aspect_ratio::w_h_10_16: {
      static AspectRatio const r( size{ .w = 5, .h = 8 } );
      return r;
    }
    case e_named_aspect_ratio::w_h_16_9: {
      static AspectRatio const r( size{ .w = 16, .h = 9 } );
      return r;
    }
    case e_named_aspect_ratio::w_h_9_16: {
      static AspectRatio const r( size{ .w = 9, .h = 16 } );
      return r;
    }
    case e_named_aspect_ratio::w_h_1_1: {
      static AspectRatio const r( size{ .w = 1, .h = 1 } );
      return r;
    }
    case e_named_aspect_ratio::w_h_21_9: {
      static AspectRatio const r( size{ .w = 7, .h = 3 } );
      return r;
    }
    case e_named_aspect_ratio::w_h_4_3: {
      static AspectRatio const r( size{ .w = 4, .h = 3 } );
      return r;
    }
  }
}

span<AspectRatio const> AspectRatio::named_all() {
  static auto const all = [] {
    vector<AspectRatio> res;
    res.reserve( refl::enum_count<e_named_aspect_ratio> );
    for( auto const e : refl::enum_values<e_named_aspect_ratio> )
      res.push_back( from_named( e ) );
    return res;
  }();
  return all;
}

void to_str( AspectRatio const& o, string& out,
             base::tag<AspectRatio> ) {
  base::to_str( o.ratio_.w, out );
  base::to_str( ':', out );
  base::to_str( o.ratio_.h, out );
}

/****************************************************************
** Public API.
*****************************************************************/
double default_aspect_ratio_tolerance() { return 0.04; }

maybe<AspectRatio> find_closest_aspect_ratio(
    span<AspectRatio const> const ratios_all,
    AspectRatio const             target ) {
  double const tolerance = default_aspect_ratio_tolerance();
  CHECK_LE( tolerance, 1.0 );
  struct Closest {
    double      delta = {};
    AspectRatio aspect_ratio;
  };
  maybe<Closest> closest;
  for( AspectRatio const ratio : ratios_all )
    if( auto const delta = is_close( ratio, target, tolerance );
        delta )
      if( !closest.has_value() || *delta < closest->delta )
        closest = { .delta = *delta, .aspect_ratio = ratio };
  return closest.member( &Closest::aspect_ratio );
}

maybe<e_named_aspect_ratio> find_closest_named_aspect_ratio(
    AspectRatio const target ) {
  double const tolerance = default_aspect_ratio_tolerance();
  CHECK_LE( tolerance, 1.0 );
  struct Closest {
    double               delta        = {};
    e_named_aspect_ratio aspect_ratio = {};
  };
  maybe<Closest> closest;
  for( auto const e : refl::enum_values<e_named_aspect_ratio> ) {
    AspectRatio const ratio = AspectRatio::from_named( e );
    if( auto const delta = is_close( ratio, target, tolerance );
        delta )
      if( !closest.has_value() || *delta < closest->delta )
        closest = { .delta = *delta, .aspect_ratio = e };
  }
  return closest.member( &Closest::aspect_ratio );
}

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

string named_ratio_canonical_name(
    e_named_aspect_ratio const r ) {
  string_view name = refl::enum_value_name( r );
  name.remove_prefix( 4 );
  // Should never be empty because this is an enum value identi-
  // fier name.
  CHECK( !name.empty() );
  string sname( name );
  replace( sname.begin(), sname.end(), '_', ':' );
  return sname;
}

bool is_exact( Resolution const& resolution ) {
  return resolution.viewport.origin.distance_from_origin() ==
         size{};
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
      res.resolutions.push_back(
          Resolution{ .physical = physical,
                      .logical  = logical,
                      .viewport = physical_rect,
                      .scores   = {} } );
    } else {
      // Fits within the window but smaller.
      size const chosen_physical =
          logical.dimensions * logical.scale;
      rect const viewport{
        .origin = centered_in( chosen_physical, physical_rect ),
        .size   = chosen_physical };
      res.resolutions.push_back(
          Resolution{ .physical = physical,
                      .logical  = logical,
                      .viewport = viewport } );
    }
    compute_scores( res.resolutions.back() );
  }
  return res;
}

ResolutionRatings resolution_ratings(
    ResolutionAnalysis const&  analysis,
    ResolutionTolerance const& tolerance ) {
  ResolutionRatings res;

  auto sorted = analysis.resolutions;
  sort( sorted.begin(), sorted.end(),
        []( Resolution const& l, Resolution const& r ) {
          if( is_exact( l ) != is_exact( r ) )
            // Exact fits should go first.
            return is_exact( l );
          if( is_exact( l ) ) {
            // FIXME: this needs to be improved to account for
            // the angular size of pixels. Such angular size
            // might also need to be weighed in to the score for
            // inexact fits. These need to be given a `score`
            // field like the inexact fits.
            return l.logical.dimensions.area() >
                   r.logical.dimensions.area();
          } else {
            return l.scores.overall < r.scores.overall;
          }
        } );

  for( auto const& resolution : sorted ) {
    auto& where = meets_tolerance( resolution, tolerance )
                      ? res.available
                      : res.unavailable;
    where.push_back( resolution );
  }

  return res;
}

} // namespace gfx
