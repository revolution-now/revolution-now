/****************************************************************
**irand.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-11.
*
* Description: Injectable interface for random number generation.
*
*****************************************************************/
#include "irand.hpp"

// config
#include "ss/rds-helpers.rds.hpp"

using namespace std;

namespace rn {

/****************************************************************
** IRand
*****************************************************************/
void to_str( IRand const& o, string& out, base::tag<IRand> ) {
  out += "IRand@";
  out += fmt::format( "{}", static_cast<void const*>( &o ) );
}

bool IRand::bernoulli( Probability const& p ) {
  return bernoulli( p.probability );
}

double IRand::NONPORTABLE__sample_piecewise3(
    Piecewise3Dist const& dist ) {
  double const width = dist.max - dist.min;
  if( width < 1e-6 ) return dist.peak;
  double const peak   = ( dist.peak - dist.min ) / width;
  double const in_0_1 = NONPORTABLE__piecewise3(
      peak, dist.weight_min, dist.weight_max );
  // clamp just in case there are rounding errors.
  return clamp( in_0_1 * width + dist.min, dist.min, dist.max );
}

} // namespace rn
