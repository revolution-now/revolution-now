/****************************************************************
**range-helpers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-12-11.
*
* Description: Helpers for specifying ranges in config data.
*
*****************************************************************/
#include "range-helpers.hpp"

// refl
#include "refl/ext.hpp"

using namespace std;

namespace rn::config {

namespace {

using ::base::valid;
using ::base::valid_or;

}

valid_or<string> IntPercent::validate() const {
  REFL_VALIDATE(
      percent >= 0 && percent <= 100,
      "integral percents must be in the range [0, 100]." );
  return valid;
}

valid_or<string> DoublePercent::validate() const {
  REFL_VALIDATE(
      fraction >= 0.0 && fraction <= 1.0,
      "floating point percents must be in the range [0, 1.0]." );
  return valid;
}

valid_or<string> DoublePercentRange::validate() const {
  REFL_VALIDATE( lo >= 0.0 && lo <= 1.0,
                 "floating point percent `lo' must be in the "
                 "range [0, 1.0]." );
  REFL_VALIDATE( hi >= 0.0 && hi <= 1.0,
                 "floating point percent `hi' must be in the "
                 "range [0, 1.0]." );
  REFL_VALIDATE( lo <= hi,
                 "`lo' must be less or equal to `hi'" );
  return valid;
}

valid_or<string> IntRange::validate() const {
  REFL_VALIDATE( min <= max, "min must be <= max in range." );
  return valid;
}

valid_or<string> DoubleRange::validate() const {
  REFL_VALIDATE( min <= max, "min must be <= max in range." );
  return valid;
}

valid_or<string> Probability::validate() const {
  REFL_VALIDATE( probability >= 0.0,
                 "probability must be >= 0.0." );
  REFL_VALIDATE( probability <= 1.0,
                 "probability must be <= 1.0." );
  return valid;
}

valid_or<string> NormalDist::validate() const {
  REFL_VALIDATE( stddev >= 0.0, "stddev must be non-negative." );
  return valid;
}

valid_or<string> BoundedNormalDist::validate() const {
  REFL_VALIDATE( stddev >= 0.0, "stddev must be non-negative." );
  REFL_VALIDATE( min <= max, "min must <= max." );
  return valid;
}

valid_or<string> UniformDist::validate() const {
  REFL_VALIDATE( min < max, "min must < max." );
  return valid;
}

valid_or<string> Piecewise3Dist::validate() const {
  static auto const is_0_1 = []( double const d ) {
    return d >= 0.0 && d <= 1.0;
  };
  REFL_VALIDATE( is_0_1( weight_min ),
                 "weight_min must in the interval [0, 1]" );
  REFL_VALIDATE( is_0_1( weight_max ),
                 "weight_max must in the interval [0, 1]" );
  REFL_VALIDATE( min <= peak, "min must be <= peak" );
  REFL_VALIDATE( peak <= max, "peak must be <= max" );
  return valid;
}

} // namespace rn::config
