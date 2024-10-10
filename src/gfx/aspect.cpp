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

bool is_close( double const l, double const r,
               double const tolerance ) {
  // Since the ratio involves two numbers, then in the worst case
  // they could both be off by `tolerance` but in different di-
  // rections, which means that the tolerance for the ratio is
  // 2*tolerance.
  double const ratio_tolerance = tolerance * 2;
  double const ratio           = l / r;
  return ( ratio >= 1.0 / ( 1.0 + ratio_tolerance ) &&
           ratio <= 1.0 + ratio_tolerance );
}

bool is_close( AspectRatio const l, AspectRatio const r,
               double const tolerance ) {
  double const l_ratio = l.scalar();
  double const r_ratio = r.scalar();
  return is_close( l_ratio, r_ratio, tolerance );
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

std::span<AspectRatio const> AspectRatio::named_all() {
  static auto const all = [] {
    vector<AspectRatio> res;
    res.reserve( refl::enum_count<e_named_aspect_ratio> );
    for( auto const e : refl::enum_values<e_named_aspect_ratio> )
      res.push_back( from_named( e ) );
    return res;
  }();
  return all;
}

void to_str( AspectRatio const& o, std::string& out,
             base::ADL_t adl ) {
  to_str( o.ratio_.w, out, adl );
  to_str( ':', out, adl );
  to_str( o.ratio_.h, out, adl );
}

/****************************************************************
** Public API.
*****************************************************************/
double default_aspect_ratio_tolerance() { return 0.04; }

maybe<AspectRatio> find_closest_aspect_ratio(
    span<AspectRatio const> const ratios_all,
    AspectRatio const target, double const tolerance ) {
  CHECK_LE( tolerance, 1.0 );
  for( AspectRatio const ratio : ratios_all )
    if( is_close( ratio, target, tolerance ) ) //
      return ratio;
  return nothing;
}

base::maybe<e_named_aspect_ratio>
find_closest_named_aspect_ratio( AspectRatio const target,
                                 double const      tolerance ) {
  for( auto const e : refl::enum_values<e_named_aspect_ratio> ) {
    AspectRatio const ratio = AspectRatio::from_named( e );
    if( is_close( ratio, target, tolerance ) ) //
      return e;
  }
  return nothing;
}

vector<size> supported_logical_resolutions(
    size const max_resolution ) {
  vector<size> resolutions;
  auto const   min_dimension =
      std::min( max_resolution.w, max_resolution.h );
  for( int i = 1; i <= min_dimension; ++i )
    if( max_resolution.w % i == 0 && max_resolution.h % i == 0 )
      resolutions.push_back( max_resolution / i );
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

} // namespace gfx
