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

base::maybe<double> scalar_ratio( size const sz ) {
  if( sz.h <= 0 ) return base::nothing;
  return sz.w / double( sz.h );
}

maybe<double> is_close( size const l, size const r,
                        double const tolerance ) {
  auto const l_scalar = scalar_ratio( l );
  auto const r_scalar = scalar_ratio( r );
  if( !l_scalar.has_value() || !r_scalar.has_value() )
    return nothing;
  // Since the ratio involves two numbers, then in the worst case
  // they could both be off by `tolerance` but in different di-
  // rections, which means that the tolerance for the ratio is
  // 2*tolerance.
  double const ratio_tolerance = tolerance * 2;
  double const ratio           = *l_scalar / *r_scalar;
  if( ratio >= 1.0 / ( 1.0 + ratio_tolerance ) &&
      ratio <= 1.0 + ratio_tolerance )
    return abs( ratio - 1.0 );
  return nothing;
}

} // namespace

/****************************************************************
** e_named_aspect_ratio
*****************************************************************/
size named_aspect_ratio( e_named_aspect_ratio const ratio ) {
  switch( ratio ) {
    case e_named_aspect_ratio::_16x10:
      return size{ .w = 16, .h = 10 };
    case e_named_aspect_ratio::_16x9:
      return size{ .w = 16, .h = 9 };
    case e_named_aspect_ratio::_21x9:
      return size{ .w = 21, .h = 9 };
    case e_named_aspect_ratio::_4x3:
      return size{ .w = 4, .h = 3 };
  }
}

/****************************************************************
** Public API.
*****************************************************************/
maybe<e_named_aspect_ratio> find_close_named_aspect_ratio(
    size const target, double const tolerance_ ) {
  double const tolerance = clamp( tolerance_, 0.0, 1.0 );
  struct Closest {
    double delta                      = {};
    e_named_aspect_ratio aspect_ratio = {};
  };
  maybe<Closest> closest;
  for( auto const e : refl::enum_values<e_named_aspect_ratio> ) {
    size const ratio = named_aspect_ratio( e );
    if( auto const delta = is_close( ratio, target, tolerance );
        delta )
      if( !closest.has_value() || *delta < closest->delta )
        closest = { .delta = *delta, .aspect_ratio = e };
  }
  return closest.member( &Closest::aspect_ratio );
}

} // namespace gfx
