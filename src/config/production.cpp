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

base::valid_or<string> CenterSquareProduction::validate() const {
  REFL_VALIDATE(
      food_bonus_by_difficulty[e_difficulty::viceroy] == 0,
      "The food_bonus_by_difficulty must be zero for viceroy." );
  REFL_VALIDATE(
      secondary_bonus_by_difficulty[e_difficulty::viceroy] == 0,
      "The secondary_bonus_by_difficulty must be zero for "
      "viceroy." );
  return base::valid;
}

} // namespace rn
