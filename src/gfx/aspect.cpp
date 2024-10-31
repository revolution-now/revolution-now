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
  auto const l_scalar = l.scalar();
  auto const r_scalar = r.scalar();
  if( !l_scalar.has_value() || !r_scalar.has_value() )
    return nothing;
  return is_close( *l_scalar, *r_scalar, tolerance );
}

// Returns the default tolerance used for bucketing of aspect ra-
// tios. This is a small positive number < 1.
double default_aspect_ratio_tolerance() { return 0.04; }

} // namespace

/****************************************************************
** AspectRatio
*****************************************************************/
void to_str( AspectRatio const& o, string& out,
             base::tag<AspectRatio> ) {
  base::to_str( o.ratio_.w, out );
  base::to_str( ':', out );
  base::to_str( o.ratio_.h, out );
}

/****************************************************************
** Public API.
*****************************************************************/
AspectRatio named_aspect_ratio(
    e_named_aspect_ratio const ratio ) {
  switch( ratio ) {
    case e_named_aspect_ratio::_16_10: {
      static AspectRatio const r( size{ .w = 8, .h = 5 } );
      return r;
    }
    case e_named_aspect_ratio::_16_9: {
      static AspectRatio const r( size{ .w = 16, .h = 9 } );
      return r;
    }
    case e_named_aspect_ratio::_21_9: {
      static AspectRatio const r( size{ .w = 7, .h = 3 } );
      return r;
    }
    case e_named_aspect_ratio::_4_3: {
      static AspectRatio const r( size{ .w = 4, .h = 3 } );
      return r;
    }
  }
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
    AspectRatio const ratio = named_aspect_ratio( e );
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
  name.remove_prefix( 1 );
  // Should never be empty because this is an enum value identi-
  // fier name.
  CHECK( !name.empty() );
  string sname( name );
  replace( sname.begin(), sname.end(), '_', ':' );
  return sname;
}

} // namespace gfx
