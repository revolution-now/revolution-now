/****************************************************************
**old-world.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-05.
*
# Description: Config info for things in the Old World.
*
*****************************************************************/
#include "old-world.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

base::valid_or<string> config::old_world::IntRange::validate()
    const {
  REFL_VALIDATE( min <= max, "min must be <= max in range." );
  return base::valid;
}

base::valid_or<string> config::old_world::DoubleRange::validate()
    const {
  REFL_VALIDATE( min <= max, "min must be <= max in range." );
  return base::valid;
}

base::valid_or<string> config::old_world::Taxes::validate()
    const {
  REFL_VALIDATE( tax_increase_probability >= 0,
                 "probabilities must be >= 0." );
  REFL_VALIDATE( tax_increase_probability <= 1.0,
                 "probabilities must be <= 1.0." );
  REFL_VALIDATE( intervention_increase_probability >= 0,
                 "probabilities must be >= 0." );
  REFL_VALIDATE( intervention_increase_probability <= 1.0,
                 "probabilities must be <= 1.0." );
  return base::valid;
}

} // namespace rn
