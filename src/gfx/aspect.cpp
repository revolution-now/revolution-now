/****************************************************************
**aspect.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-09.
*
* Description: Helpers for dealing with aspect ratios.
*
*****************************************************************/
#include "aspect.hpp"

// refl
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

} // namespace gfx
