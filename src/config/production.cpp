/****************************************************************
**production.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-02.
*
# Description: Config info for colony production.
*
*****************************************************************/
#include "production.hpp"

// refl
#include "refl/ext.hpp"

using namespace std;

namespace rn {

base::valid_or<string> config_production_t::validate() const {
  return base::valid;
}

base::valid_or<string> OutdoorJobBonus::none::validate() const {
  return base::valid;
}

base::valid_or<string> OutdoorJobBonus::add::validate() const {
  REFL_VALIDATE(
      expert >= non_expert,
      "expert must be >= non_expert for an outdoor job bonus." );
  return base::valid;
}

base::valid_or<string> OutdoorJobBonus::mul::validate() const {
  return base::valid;
}

} // namespace rn
