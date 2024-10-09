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
    case e_named_aspect_ratio::_16x10:
      return AspectRatio( size{ .w = 8, .h = 5 } );
    case e_named_aspect_ratio::_16x9:
      return AspectRatio( size{ .w = 16, .h = 9 } );
    case e_named_aspect_ratio::_1x1:
      return AspectRatio( size{ .w = 1, .h = 1 } );
    case e_named_aspect_ratio::_21x9:
      return AspectRatio( size{ .w = 7, .h = 3 } );
    case e_named_aspect_ratio::_4x3:
      return AspectRatio( size{ .w = 4, .h = 3 } );
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
double default_ratio_tolerance() { return 0.05; }

maybe<AspectRatio> find_closest_aspect_ratio(
    span<AspectRatio const> const ratios_all,
    AspectRatio const target, double const tolerance ) {
  CHECK_LE( tolerance, 1.0 );
  for( AspectRatio const ratio : ratios_all ) {
    double const ratio_of_ratios =
        ratio.scalar() / target.scalar();
    if( ratio_of_ratios >= 1.0 - tolerance &&
        ratio_of_ratios <= 1.0 + tolerance )
      return ratio;
  }
  return nothing;
}

} // namespace gfx
