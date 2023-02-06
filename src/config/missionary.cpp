/****************************************************************
**missionary.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-01-19.
*
* Description: config data for things related to missionaries.
*
*****************************************************************/
#include "missionary.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

base::valid_or<string> config_missionary_t::validate() const {
  REFL_VALIDATE( type[e_missionary_type::normal].strength == 1.0,
                 "strength of '{}' missionary type must be 1.0.",
                 e_missionary_type::normal );

  REFL_VALIDATE( saturation_reduction_for_non_jesuit_cross < 1.0,
                 "saturation_reduction_for_non_jesuit_cross "
                 "must be less than 1." );
  return base::valid;
}

} // namespace rn
