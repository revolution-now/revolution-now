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
  double const l_ratio = l.scalar();
  double const r_ratio = r.scalar();
  return is_close( l_ratio, r_ratio, tolerance );
}

Resolution to_resolution( LogicalResolution const& logical ) {
  rect const target_logical_rect{ .origin = {},
                                  .size   = logical.resolution };
  return Resolution{ .exact           = true,
                     .target_logical  = logical.resolution,
                     .scale           = logical.scale,
                     .clipped_logical = target_logical_rect,
                     .logical         = logical.resolution,
                     .buffer          = {},
                     .score           = 0 };
}

Resolution to_resolution(
    InexactLogicalResolution const& inexact ) {
  return Resolution{ .exact           = false,
                     .target_logical  = inexact.target_logical,
                     .scale           = inexact.scale,
                     .clipped_logical = inexact.clipped_logical,
                     .logical         = inexact.logical,
                     .buffer          = inexact.buffer,
                     .score           = inexact.score };
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
    case e_named_aspect_ratio::_16x10: {
      static AspectRatio const r( size{ .w = 8, .h = 5 } );
      return r;
    }
    case e_named_aspect_ratio::_16x9: {
      static AspectRatio const r( size{ .w = 16, .h = 9 } );
      return r;
    }
    case e_named_aspect_ratio::_1x1: {
      static AspectRatio const r( size{ .w = 1, .h = 1 } );
      return r;
    }
    case e_named_aspect_ratio::_21x9: {
      static AspectRatio const r( size{ .w = 7, .h = 3 } );
      return r;
    }
    case e_named_aspect_ratio::_4x3: {
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
    AspectRatio const target, double const tolerance ) {
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
    AspectRatio const target, double const tolerance ) {
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
        .resolution = max_resolution / i, .scale = i } );
  return resolutions;
}

string named_ratio_canonical_name(
    e_named_aspect_ratio const r ) {
  string_view const name = refl::enum_value_name( r );
  // Should never be empty because this is an enum value identi-
  // fier name.
  CHECK( !name.empty() );

  string res( name.begin() + 1, name.end() );
  std::replace( res.begin(), res.end(), 'x', ':' );
  return res;
}

ResolutionAnalysis resolution_analysis(
    span<size const> const target_logical_resolutions,
    size const             physical ) {
  rect const physical_rect{ .origin = {}, .size = physical };
  vector<LogicalResolution> const choices =
      supported_logical_resolutions( physical );
  ResolutionAnalysis res;
  res.physical = physical;
  for( size const target : target_logical_resolutions ) {
    LogicalResolution scaled{ .resolution = target, .scale = 1 };
    maybe<LogicalResolution> largest_that_fits;
    while( scaled.resolution.fits_inside( physical ) ) {
      largest_that_fits = scaled;
      ++scaled.scale;
      scaled.resolution = target * scaled.scale;
    }
    LogicalResolution const smallest_that_does_not_fit = scaled;
    if( largest_that_fits.has_value() &&
        largest_that_fits->resolution == physical ) {
      LogicalResolution unscaled = *largest_that_fits;
      unscaled.resolution = unscaled.resolution / unscaled.scale;
      res.exact_fits.push_back( unscaled );
      continue;
    }
    auto score_for = [&]( size const l ) {
      CHECK_GT( l.area(), 0 );
      return ( l - physical ).pythagorean() /
             physical.pythagorean();
    };

    // Did not exactly fit, so we'll add the one that is just
    // smaller and just larger, since at least one of those is
    // always better than any of the others.
    auto add_inexact = [&]( LogicalResolution const inexact ) {
      double const score = score_for( inexact.resolution );
      CHECK_GT( score, 0.0 );
      point const virtual_ne_physical =
          centered_in( inexact.resolution, physical_rect );
      rect const virtual_rect_physical{
        .origin = virtual_ne_physical,
        .size   = inexact.resolution };
      UNWRAP_CHECK_T(
          auto const clipped_physical,
          virtual_rect_physical.clipped_by( physical_rect ) );
      auto const clipped_logical =
          clipped_physical / inexact.scale;
      auto const virtual_ne_logical =
          virtual_ne_physical / inexact.scale;
      auto const buffer_logical =
          virtual_ne_logical.distance_from_origin();
      auto const logical = inexact.resolution / inexact.scale;
      res.inexact_fits.push_back( InexactLogicalResolution{
        .target_logical  = target,
        .scale           = inexact.scale,
        .clipped_logical = clipped_logical,
        .logical         = logical,
        .buffer          = buffer_logical,
        .score           = score } );
    };

    add_inexact( smallest_that_does_not_fit );
    if( largest_that_fits.has_value() )
      add_inexact( *largest_that_fits );
  }
  return res;
}

static bool meets_tolerance(
    size const physical, InexactLogicalResolution const& inexact,
    ResolutionTolerance const& tolerance ) {
  if( tolerance.max_missing_pixels.has_value() ) {
    if( -inexact.buffer.w > *tolerance.max_missing_pixels ||
        -inexact.buffer.h > *tolerance.max_missing_pixels )
      return false;
  }

  if( tolerance.max_extra_pixels.has_value() ) {
    if( inexact.buffer.w > *tolerance.max_extra_pixels ||
        inexact.buffer.h > *tolerance.max_extra_pixels )
      return false;
  }

  if( tolerance.min_percent_covered.has_value() ) {
    double const percent_covered =
        inexact.clipped_logical.area() /
        static_cast<double>( physical.area() );
    if( percent_covered < *tolerance.min_percent_covered )
      return false;
  }

  if( tolerance.score_cutoff.has_value() ) {
    if( inexact.score > *tolerance.score_cutoff ) return false;
  }
  return true;
}

vector<Resolution> available_resolutions(
    ResolutionAnalysis const&  analysis,
    ResolutionTolerance const& tolerance ) {
  vector<Resolution> res;

  auto const ordered_exact_fits = [&] {
    auto sorted = analysis.exact_fits;
    sort( sorted.begin(), sorted.end(),
          []( LogicalResolution const& l,
              LogicalResolution const& r ) {
            // FIXME: this needs to be improved to account for
            // the angular size of pixels. Such angular size
            // might also need to be weighed in to the score for
            // inexact fits. These need to be given a `score`
            // field like the inexact fits.
            return l.resolution.area() > r.resolution.area();
          } );
    return sorted;
  }();

  auto const ordered_inexact_fits = [&] {
    auto sorted = analysis.inexact_fits;
    sort( sorted.begin(), sorted.end(),
          []( InexactLogicalResolution const& l,
              InexactLogicalResolution const& r ) {
            return l.score < r.score;
          } );
    return sorted;
  }();

  // Exact fits should go first.
  for( auto const& exact_fit : ordered_exact_fits )
    res.push_back( to_resolution( exact_fit ) );

  for( auto const& inexact_fit : ordered_inexact_fits ) {
    if( !meets_tolerance( analysis.physical, inexact_fit,
                          tolerance ) )
      continue;
    // FIXME: this additional condition of the kind of buffer
    // needs to be weighed in during sorting into the score. We
    // may also want to weigh in the angular pixel size as we
    // probably should for the scoring of exact fits.
    // if( !inexact_fit.buffer.negative() ) {
    //   res.push_back( to_resolution( inexact_fit ) );
    // }
    res.push_back( to_resolution( inexact_fit ) );
  }

  // Populate physical resolution.
  for( auto& available : res )
    available.physical = analysis.physical;

  return res;
}

base::maybe<Resolution> recommended_resolution(
    ResolutionAnalysis const&  analysis,
    ResolutionTolerance const& tolerance ) {
  auto available = available_resolutions( analysis, tolerance );
  if( available.empty() ) return nothing;
  return std::move( available[0] );
}

} // namespace gfx
