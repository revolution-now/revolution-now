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

base::valid_or<string> IntRange::validate() const {
  REFL_VALIDATE( min <= max, "min must be <= max in range." );
  return base::valid;
}

base::valid_or<string> DoubleRange::validate() const {
  REFL_VALIDATE( min <= max, "min must be <= max in range." );
  return base::valid;
}

base::valid_or<string> Probability::validate() const {
  REFL_VALIDATE( probability >= 0.0,
                 "probability must be >= 0.0." );
  REFL_VALIDATE( probability <= 1.0,
                 "probability must be <= 1.0." );
  return base::valid;
}

} // namespace rn::config
